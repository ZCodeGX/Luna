#ifndef LUNA_HPP
#define LUNA_HPP

#include <luna/luna.h>
#include <string>
#include <stdexcept>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <new>

namespace luna {

/* ---------- Error ---------- */
class Error : public std::runtime_error {
public:
    Error(LunaStatus s, std::string msg)
        : std::runtime_error(std::move(msg)), status_(s) {}
    LunaStatus status() const noexcept { return status_; }
private:
    LunaStatus status_;
};

/* ---------- Traits para deducir tipos de un callable ---------- */
namespace detail {

template <typename T> struct function_traits;

template <typename R, typename... Args>
struct function_traits<R(Args...)> {
    using return_type = R;
    static constexpr size_t arity = sizeof...(Args);
    template <size_t I>
    using arg = std::tuple_element_t<I, std::tuple<Args...>>;
};

template <typename R, typename... Args>
struct function_traits<R(*)(Args...)> : function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...)> : function_traits<R(Args...)> {};

template <typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...) const> : function_traits<R(Args...)> {};

template <typename F>
struct function_traits : function_traits<decltype(&F::operator())> {};

/* ---------- Lectura de argumentos desde Luna ---------- */
template <typename T>
T read(LunaState* L, int i) {
    if constexpr (std::is_same_v<T, std::string>) {
        size_t len = 0;
        const char* s = luna_checkstring(L, i, &len);
        return std::string(s, len);
    } else if constexpr (std::is_same_v<T, int>) {
        return static_cast<int>(luna_checkinteger(L, i));
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return luna_checkinteger(L, i);
    } else if constexpr (std::is_same_v<T, double>) {
        return luna_checknumber(L, i);
    } else if constexpr (std::is_same_v<T, bool>) {
        return luna_checkboolean(L, i) != 0;
    } else {
        static_assert(sizeof(T) == 0, "unsupported argument type");
    }
}

/* ---------- Escritura de un valor en Luna ---------- */
inline void write(LunaState* L, int v)               { luna_pushinteger(L, v); }
inline void write(LunaState* L, int64_t v)           { luna_pushinteger(L, v); }
inline void write(LunaState* L, double v)            { luna_pushfloat(L, v); }
inline void write(LunaState* L, bool v)              { luna_pushboolean(L, v ? 1 : 0); }
inline void write(LunaState* L, const std::string& s){ luna_pushlstring(L, s.data(), s.size()); }
inline void write(LunaState* L, const char* s)       { luna_pushstring(L, s); }

/* ---------- Trampolín: envuelve un callable C++ ---------- */
template <typename Fn, typename Traits, size_t... Is>
int trampoline_impl(LunaState* L, Fn* fn, std::index_sequence<Is...>) {
    using R = typename Traits::return_type;
    try {
        if constexpr (std::is_void_v<R>) {
            (*fn)(read<typename Traits::template arg<Is>>(L, (int)Is + 1)...);
            return 0;
        } else {
            R result = (*fn)(read<typename Traits::template arg<Is>>(L, (int)Is + 1)...);
            write(L, result);
            return 1;
        }
    } catch (const std::exception& e) {
        luna_pushstring(L, e.what());
        return -1;
    } catch (...) {
        luna_pushstring(L, "unknown C++ exception");
        return -1;
    }
}

template <typename Fn>
int trampoline(LunaState* L) {
    using Traits = function_traits<Fn>;
    luna_getupvalue(L, 1, 1);
    void* ud = luna_touserdata(L, -1);
    luna_pop(L, 1);
    if (!ud) {
        luna_pushstring(L, "invalid userdata in native function");
        return -1;
    }
    return trampoline_impl<Fn, Traits>(L, static_cast<Fn*>(ud),
                                       std::make_index_sequence<Traits::arity>{});
}

/* ---------- Wrapper que captura el tipo exacto de Fn en el finalizer ---------- */
template <typename Fn>
void destroy_fn(LunaState*, void* p) { static_cast<Fn*>(p)->~Fn(); }

} // namespace detail

/* ---------- State ---------- */
class State {
public:
    State()                       : L_(luna_new(nullptr)) { check_alloc(); }
    explicit State(LunaRuntime* r): L_(luna_new(r))       { check_alloc(); }

    ~State() { if (L_) luna_close(L_); }

    State(const State&)            = delete;
    State& operator=(const State&) = delete;
    State(State&& o) noexcept : L_(o.L_) { o.L_ = nullptr; }
    State& operator=(State&& o) noexcept { std::swap(L_, o.L_); return *this; }

    LunaState* raw() noexcept { return L_; }

    /* ---- Ejecución ---- */
    void load(const std::string& path) { check(luna_dofile(L_, path.c_str())); }
    void do_string(const std::string& src, const char* name = "=string") {
        check(luna_dostring(L_, src.c_str(), name));
    }

    /* ---- Llamada a función global con tipos inferidos ---- */
    template <typename R, typename... Args>
    R call(const std::string& name, Args&&... args) {
        luna_getglobal(L_, name.c_str());
        const int base = luna_gettop(L_) - 1;
        (push(std::forward<Args>(args)), ...);
        check(luna_pcall(L_, (int)sizeof...(Args), 1, 0));
        R out = get<R>(-1);
        luna_settop(L_, base);
        return out;
    }

    template <typename... Args>
    void call_void(const std::string& name, Args&&... args) {
        luna_getglobal(L_, name.c_str());
        const int base = luna_gettop(L_) - 1;
        (push(std::forward<Args>(args)), ...);
        check(luna_pcall(L_, (int)sizeof...(Args), 0, 0));
        luna_settop(L_, base);
    }

    /* ---- Registro de funciones nativas ---- */
    template <typename F>
    void register_function(const std::string& name, F&& fn) {
        using Fn = std::decay_t<F>;
        void* box = luna_newuserdata_finalized(L_, sizeof(Fn), &detail::destroy_fn<Fn>);
        new (box) Fn(std::forward<F>(fn));
        luna_pushcclosure(L_, &detail::trampoline<Fn>, 1);
        luna_setglobal(L_, name.c_str());
    }

    /* ---- Globals ---- */
    void set_global(const std::string& name) { luna_setglobal(L_, name.c_str()); }

    template <typename T>
    T get_global(const std::string& name) {
        luna_getglobal(L_, name.c_str());
        T out = get<T>(-1);
        luna_pop(L_, 1);
        return out;
    }

    /* ---- Push ---- */
    void push(std::nullptr_t)         { luna_pushnil(L_); }
    void push(bool v)                 { luna_pushboolean(L_, v ? 1 : 0); }
    void push(int v)                  { luna_pushinteger(L_, v); }
    void push(int64_t v)              { luna_pushinteger(L_, v); }
    void push(double v)               { luna_pushfloat(L_, v); }
    void push(const std::string& s)   { luna_pushlstring(L_, s.data(), s.size()); }
    void push(const char* s)          { luna_pushstring(L_, s); }
    void push(LunaCFunction f)        { luna_pushcfunction(L_, f); }

    /* ---- Get ---- */
    template <typename T>
    T get(int index) {
        if constexpr (std::is_same_v<T, bool>)        return luna_toboolean(L_, index) != 0;
        else if constexpr (std::is_same_v<T, int>)    return (int)detail::read<int64_t>(L_, index);
        else if constexpr (std::is_same_v<T, int64_t>)return detail::read<int64_t>(L_, index);
        else if constexpr (std::is_same_v<T, double>) return detail::read<double>(L_, index);
        else if constexpr (std::is_same_v<T, std::string>) return detail::read<std::string>(L_, index);
        else static_assert(sizeof(T) == 0, "unsupported return type");
    }

private:
    void check_alloc() { if (!L_) throw std::bad_alloc(); }

    void check(LunaStatus s) {
        if (s != LUNA_OK) {
            const char* msg = luna_last_error(L_);
            throw Error(s, msg ? msg : "unknown error");
        }
    }

    LunaState* L_ = nullptr;
};

} // namespace luna

#endif /* LUNA_HPP */