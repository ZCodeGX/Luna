(function () {
  const root = document.documentElement;
  const themeToggle = document.getElementById('theme-toggle');
  const menuToggle = document.getElementById('menu-toggle');
  const sidebar = document.getElementById('sidebar');
  const search = document.getElementById('search');
  const searchResults = document.getElementById('search-results');
  const toTop = document.getElementById('to-top');
  const liveRegion = document.getElementById('live-region');
  const navLinks = Array.from(sidebar.querySelectorAll('a[href^="#"]'));
  const sections = navLinks
    .map((link) => document.querySelector(link.getAttribute('href')))
    .filter(Boolean);
  const checkpoints = Array.from(document.querySelectorAll('.checkpoint'));
  const progressRail = document.getElementById('progress-rail');
  const progressFill = document.getElementById('progress-fill');
  const routeProgress = document.getElementById('route-progress');
  const routeReset = document.getElementById('route-reset');
  const ROUTE_KEY = 'luna-docs-route';
  let resultLinks = [];
  let resultIndex = -1;

  function announce(message) {
    if (liveRegion) liveRegion.textContent = message;
  }

  try {
    const saved = localStorage.getItem('luna-docs-theme');
    if (saved) root.setAttribute('data-theme', saved);
    else if (window.matchMedia('(prefers-color-scheme: dark)').matches) root.setAttribute('data-theme', 'dark');
  } catch (error) {
    root.setAttribute('data-theme', 'light');
  }

  themeToggle.addEventListener('click', function () {
    const next = root.getAttribute('data-theme') === 'dark' ? 'light' : 'dark';
    root.setAttribute('data-theme', next);
    try {
      localStorage.setItem('luna-docs-theme', next);
    } catch (error) {
      root.setAttribute('data-theme', next);
    }
    announce(next === 'dark' ? 'Tema oscuro activado' : 'Tema claro activado');
  });

  function closeMenu() {
    sidebar.classList.remove('open');
    menuToggle.setAttribute('aria-expanded', 'false');
  }

  menuToggle.addEventListener('click', function () {
    const open = sidebar.classList.toggle('open');
    menuToggle.setAttribute('aria-expanded', String(open));
  });

  navLinks.forEach(function (link) {
    link.addEventListener('click', closeMenu);
  });

  document.addEventListener('click', function (event) {
    if (!sidebar.classList.contains('open')) return;
    if (sidebar.contains(event.target) || menuToggle.contains(event.target)) return;
    closeMenu();
  });

  let targetCounter = 0;
  function normalized(text) {
    return text.replace(/\s+/g, ' ').trim();
  }

  function targetId(element) {
    if (element.id) return element.id;
    element.id = 'idx-' + (++targetCounter);
    return element.id;
  }

  const searchIndex = [];
  Array.from(document.querySelectorAll('main section[id]')).forEach(function (section) {
    const heading = section.querySelector('h1, h2');
    searchIndex.push({
      id: section.id,
      title: heading ? normalized(heading.textContent) : section.id,
      text: normalized(section.textContent),
      kind: 'section'
    });
  });

  Array.from(document.querySelectorAll('main section[id] h3, main section[id] summary, main section[id] pre code')).forEach(function (element) {
    const section = element.closest('section[id]');
    if (!section) return;
    const raw = normalized(element.textContent);
    if (!raw) return;
    const isCode = element.parentElement && element.parentElement.tagName === 'PRE';
    const firstLine = raw.split('\n')[0].trim();
    const title = isCode
      ? 'Código · ' + firstLine.slice(0, 64)
      : element.tagName === 'SUMMARY' ? 'Solución · ' + firstLine.slice(0, 64) : firstLine.slice(0, 64);
    searchIndex.push({
      id: targetId(isCode ? element.parentElement : element),
      title: title,
      text: raw + ' · ' + section.id,
      kind: isCode ? 'code' : element.tagName === 'SUMMARY' ? 'solution' : 'subsection'
    });
  });

  function hideResults() {
    searchResults.hidden = true;
    search.setAttribute('aria-expanded', 'false');
    resultLinks = [];
    resultIndex = -1;
  }

  function showResults() {
    searchResults.hidden = false;
    search.setAttribute('aria-expanded', 'true');
  }

  function snippet(entry, terms) {
    const lower = entry.text.toLowerCase();
    let position = -1;
    terms.forEach(function (term) {
      if (position === -1) position = lower.indexOf(term);
    });
    if (position === -1) return entry.text.slice(0, 90);
    const start = Math.max(0, position - 30);
    const end = Math.min(entry.text.length, position + 90);
    return (start > 0 ? '…' : '') + entry.text.slice(start, end).trim() + (end < entry.text.length ? '…' : '');
  }

  function setResultSelection(next) {
    if (!resultLinks.length) return;
    if (resultIndex >= 0) resultLinks[resultIndex].classList.remove('selected');
    resultIndex = (next + resultLinks.length) % resultLinks.length;
    resultLinks[resultIndex].classList.add('selected');
  }

  function updateSearch() {
    const query = search.value.trim().toLowerCase();
    if (!query) {
      hideResults();
      return;
    }
    const terms = query.split(/\s+/).filter(Boolean);
    const matches = searchIndex.filter(function (entry) {
      const lower = entry.text.toLowerCase();
      return terms.every(function (term) { return lower.indexOf(term) !== -1; });
    });
    matches.sort(function (a, b) {
      const aTitle = a.title.toLowerCase().indexOf(terms[0]);
      const bTitle = b.title.toLowerCase().indexOf(terms[0]);
      if (aTitle !== bTitle) return aTitle < 0 ? 1 : bTitle < 0 ? -1 : aTitle - bTitle;
      return a.title.localeCompare(b.title);
    });

    searchResults.textContent = '';
    resultLinks = [];
    resultIndex = -1;

    if (!matches.length) {
      const empty = document.createElement('p');
      empty.textContent = 'Sin resultados para «' + search.value.trim() + '»';
      searchResults.appendChild(empty);
      showResults();
      announce('Sin resultados de búsqueda');
      return;
    }

    matches.slice(0, 9).forEach(function (entry) {
      const item = document.createElement('li');
      const link = document.createElement('a');
      link.href = '#' + entry.id;
      link.dataset.kind = entry.kind;
      const title = document.createElement('strong');
      title.textContent = entry.title;
      const detail = document.createElement('small');
      detail.textContent = snippet(entry, terms);
      link.appendChild(title);
      link.appendChild(detail);
      link.addEventListener('click', function () {
        hideResults();
        closeMenu();
        announce('Saltando a ' + entry.title);
      });
      item.appendChild(link);
      searchResults.appendChild(item);
      resultLinks.push(link);
    });
    showResults();
    announce(resultLinks.length + ' resultados de búsqueda');
  }

  search.addEventListener('input', updateSearch);

  search.addEventListener('keydown', function (event) {
    if (event.key === 'ArrowDown') {
      event.preventDefault();
      setResultSelection(resultIndex + 1);
    } else if (event.key === 'ArrowUp') {
      event.preventDefault();
      setResultSelection(resultIndex - 1);
    } else if (event.key === 'Enter' && resultIndex >= 0) {
      event.preventDefault();
      resultLinks[resultIndex].click();
    } else if (event.key === 'Escape') {
      hideResults();
      search.blur();
    }
  });

  document.addEventListener('click', function (event) {
    if (!searchResults.hidden && !search.contains(event.target) && !searchResults.contains(event.target)) hideResults();
  });

  document.addEventListener('keydown', function (event) {
    if ((event.ctrlKey || event.metaKey) && (event.key === 'k' || event.key === 'K')) {
      event.preventDefault();
      search.focus();
      search.select();
    }
    if (event.key === 'Escape') {
      if (!searchResults.hidden) hideResults();
      if (sidebar.classList.contains('open')) {
        closeMenu();
        menuToggle.focus();
      }
    }
  });

  function setActive(id) {
    navLinks.forEach(function (link) {
      const active = link.getAttribute('href') === '#' + id;
      link.classList.toggle('active', active);
      if (active) link.setAttribute('aria-current', 'true');
      else link.removeAttribute('aria-current');
    });
  }

  if ('IntersectionObserver' in window) {
    const observer = new IntersectionObserver(function (entries) {
      entries.forEach(function (entry) {
        if (entry.isIntersecting) setActive(entry.target.id);
      });
    }, { rootMargin: '-20% 0px -70% 0px' });
    sections.forEach(function (section) { observer.observe(section); });
  } else {
    window.addEventListener('scroll', function () {
      let current = sections[0] ? sections[0].id : '';
      sections.forEach(function (section) {
        if (section.getBoundingClientRect().top <= 120) current = section.id;
      });
      setActive(current);
    }, { passive: true });
  }

  document.querySelectorAll('pre').forEach(function (pre) {
    if (pre.querySelector('.copy')) return;
    const button = document.createElement('button');
    button.className = 'copy';
    button.type = 'button';
    button.textContent = 'Copiar';
    pre.insertBefore(button, pre.firstChild);
  });

  function legacyCopy(text) {
    const area = document.createElement('textarea');
    area.value = text;
    area.setAttribute('readonly', '');
    area.style.position = 'fixed';
    area.style.opacity = '0';
    document.body.appendChild(area);
    area.select();
    let copied = false;
    try {
      copied = document.execCommand('copy');
    } catch (error) {
      copied = false;
    }
    document.body.removeChild(area);
    return copied;
  }

  document.querySelectorAll('.copy').forEach(function (button) {
    button.addEventListener('click', function () {
      const codeElement = button.parentElement.querySelector('code');
      if (!codeElement) return;
      const clone = codeElement.cloneNode(true);
      clone.querySelectorAll('.copy').forEach(function (node) { node.remove(); });
      const code = clone.textContent.replace(/\u00a0/g, ' ').trim();

      function feedback(message) {
        button.textContent = message;
        announce(message);
        setTimeout(function () { button.textContent = 'Copiar'; }, 1500);
      }

      if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(code)
          .then(function () { feedback('Copiado'); })
          .catch(function () { feedback(legacyCopy(code) ? 'Copiado' : 'Selecciona y copia'); });
      } else {
        feedback(legacyCopy(code) ? 'Copiado' : 'Selecciona y copia');
      }
    });
  });

  function loadRoute() {
    try {
      const raw = localStorage.getItem(ROUTE_KEY);
      const parsed = raw ? JSON.parse(raw) : [];
      return Array.isArray(parsed) ? parsed : [];
    } catch (error) {
      return [];
    }
  }

  function updateRoute() {
    const done = checkpoints.filter(function (box) { return box.checked; });
    checkpoints.forEach(function (box) {
      const milestone = box.closest('.milestone');
      if (milestone) milestone.classList.toggle('done', box.checked);
    });
    const percent = checkpoints.length ? Math.round(done.length / checkpoints.length * 100) : 0;
    progressFill.style.width = percent + '%';
    progressRail.setAttribute('aria-valuenow', String(done.length));
    routeProgress.textContent = done.length + ' de ' + checkpoints.length + ' hitos completados';
    try {
      localStorage.setItem(ROUTE_KEY, JSON.stringify(done.map(function (box) { return box.dataset.checkpoint; })));
    } catch (error) {
      announce('No se pudo guardar el progreso en este navegador');
    }
  }

  const savedRoute = loadRoute();
  checkpoints.forEach(function (box) {
    box.checked = savedRoute.indexOf(box.dataset.checkpoint) !== -1;
    box.addEventListener('change', function () {
      updateRoute();
      announce(box.checked ? 'Hito completado' : 'Hito marcado como pendiente');
    });
  });
  updateRoute();

  routeReset.addEventListener('click', function () {
    checkpoints.forEach(function (box) { box.checked = false; });
    updateRoute();
    announce('Ruta reiniciada');
  });

  window.addEventListener('scroll', function () {
    toTop.classList.toggle('visible', window.scrollY > 400);
  }, { passive: true });

  toTop.addEventListener('click', function () {
    window.scrollTo({ top: 0, behavior: 'smooth' });
  });
})();
