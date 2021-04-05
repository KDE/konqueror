function hasRefreshAttribute() {
  const metas = document.getElementsByTagName("meta");
  for (const m of metas) {
    if (m.getAttribute("http-equiv") == "refresh") return true;
  }
  return false;
}
