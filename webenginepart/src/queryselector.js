function querySelectorAllToList(sel) {
  var list = document.querySelectorAll(sel);
  var result = [];
  for (const e of list) {
    var obj = {"tag": e.tagName, "attributes": {}};
    for (const a of e.attributes) {
      obj.attributes[a.name] = a.value;
    }
    result.push(obj);
  }
  return result;
}

function querySelectorToObject(sel) {
  var el = document.querySelector(sel);
  var result = {};
  if (el) {
    result.tag = el.tagName;
    result.attributes = {};
    for (const a of el.attributes) {
      result.attributes[a.name] = a.value;
    }
  }
  return result;
}
