var content = '%2';
var style = document.getElementById('%1');
//If `content` is not empty, use it as the content of the style element, creating the element if needed
//If `content` is empty, it means that no custom stylesheet should be used, so remove the element
if (content.length > 0) {
  if (!style) {
    style = document.createElement("style");
    style.id = '%1';
    document.head.appendChild(style);
  }
  //Set the new stylesheet
  style.innerText = '%2';
} else if (style) {
  style.remove();
}
