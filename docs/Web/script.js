(function () {
  const root = document.documentElement;
  const themeToggle = document.getElementById('theme-toggle');
  const menuToggle = document.getElementById('menu-toggle');
  const sidebar = document.getElementById('sidebar');
  const search = document.getElementById('search');
  const toTop = document.getElementById('to-top');
  const navLinks = Array.from(sidebar.querySelectorAll('a[href^="#"]'));
  const sections = navLinks
    .map((link) => document.querySelector(link.getAttribute('href')))
    .filter(Boolean);

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
  });

  menuToggle.addEventListener('click', function () {
    const open = sidebar.classList.toggle('open');
    menuToggle.setAttribute('aria-expanded', String(open));
  });

  navLinks.forEach(function (link) {
    link.addEventListener('click', function () {
      sidebar.classList.remove('open');
      menuToggle.setAttribute('aria-expanded', 'false');
    });
  });

  search.addEventListener('input', function () {
    const query = search.value.trim().toLowerCase();
    sidebar.querySelectorAll('a').forEach(function (link) {
      if (!link.getAttribute('href').startsWith('#')) return;
      link.classList.toggle('hidden', query !== '' && !link.textContent.toLowerCase().includes(query));
    });
  });

  if ('IntersectionObserver' in window) {
    const observer = new IntersectionObserver(function (entries) {
      entries.forEach(function (entry) {
        if (!entry.isIntersecting) return;
        navLinks.forEach(function (link) {
          link.classList.toggle('active', link.getAttribute('href') === '#' + entry.target.id);
        });
      });
    }, { rootMargin: '-20% 0px -70% 0px' });
    sections.forEach(function (section) { observer.observe(section); });
  }

  document.querySelectorAll('.copy').forEach(function (button) {
    button.addEventListener('click', function () {
      const codeElement = button.parentElement.querySelector('code').cloneNode(true);
      codeElement.querySelectorAll('.copy').forEach(function (node) { node.remove(); });
      const code = codeElement.textContent.trim();
      if (navigator.clipboard && navigator.clipboard.writeText) {
        navigator.clipboard.writeText(code).then(function () {
          button.textContent = 'Copiado';
          setTimeout(function () { button.textContent = 'Copiar'; }, 1500);
        });
      } else {
        button.textContent = 'Selecciona y copia';
        setTimeout(function () { button.textContent = 'Copiar'; }, 1500);
      }
    });
  });

  window.addEventListener('scroll', function () {
    toTop.classList.toggle('visible', window.scrollY > 400);
  }, { passive: true });

  toTop.addEventListener('click', function () {
    window.scrollTo({ top: 0, behavior: 'smooth' });
  });
})();
