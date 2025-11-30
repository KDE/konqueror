function updateSpeedDialIcon(entryUrl, iconUrl, localIconUrl) {
  console.log("UPDATING SPEED DIAL FOR");
  let aElems = document.getElementsByClassName(entryUrl);
  for (a of aElems) {
    let img = a.getElementsByTagName("img")[0];
    if (!img || img.className != iconUrl) {
      continue;
    }
    img.setAttribute("src", localIconUrl);
  }
}
