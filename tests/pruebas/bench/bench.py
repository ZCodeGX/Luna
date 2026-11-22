
def fibonacci(n):
    if n < 2:
        return n
    return fibonacci(n - 1) + fibonacci(n - 2)


def main():
    i = 0
    total = 0

    while i < 1_000_000:
        total += (i * 3 + 7) % 1009
        i += 1

    print("PYTHON - BUCLE")
    print(total)

    print("PYTHON - FIBONACCI")
    print(fibonacci(28))


if __name__ == "__main__":
    main()
