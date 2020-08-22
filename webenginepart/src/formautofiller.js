function labelsForIdsInFrame(frm) {
  var labelList = frm.document.getElementsByTagName("label");
  var res = new Object;
  for (var i = 0; i < labelList.length; i++) {
    var l = labelList[i];
    if (l.htmlFor != '') {
      var obj = frm.document.getElementById(l.htmlFor);
      if (obj) {
        res[obj.id] = l.innerHTML;
      }
    }
  }
  return res;
}

function findFormsRecursive(wnd, existingList, path, findLabels){
      findFormsInFrame(wnd, existingList, path, findLabels);
      var frameList = wnd.frames;
      for(var i = 0; i < frameList.length; ++i) {
          var newPath = path.concat(i);
          findFormsRecursive(frameList[i], existingList, newPath, findLabels);
      }
  }

  function findFormsInFrame(frm, existingList, path, findLabels){
      var url = frm.location;
      var formList;
      try{ formList = frm.document.forms; }
      catch(e){
        return;
      }
      var labelsForIds;
      if (findLabels) {
        labelsForIds = labelsForIdsInFrame(frm);
      }
      if (formList.length > 0) {
          for (var i = 0; i < formList.length; ++i) {
              var inputList = formList[i].elements;
              if (inputList.length < 1) {
                  continue;
              }
              var formObject = new Object;
              formObject.url = url;
              formObject.name = formList[i].name;
              if (typeof(formObject.name) != 'string') {
                  formObject.name = String(formList[i].id);
              }
              formObject.index = String(i);
              formObject.elements = new Array;
              for (var j = 0; j < inputList.length; ++j) {
                  if (inputList[j].type != 'text' && inputList[j].type != 'email' && inputList[j].type != 'password') {
                      continue;
                  }
                  var element = new Object;
                  element.id = String(inputList[j].id);
                  element.name = inputList[j].name;
                  if (typeof(element.name) != 'string' ) {
                      element.name = element.id;
                  }
                  element.value = String(inputList[j].value);
                  element.type = String(inputList[j].type);
                  element.readonly = Boolean(inputList[j].readOnly);
                  element.disabled = Boolean(inputList[j].disabled);
                  element.autocompleteAllowed = inputList[j].autocomplete != 'off';
                  if (findLabels && element.id) {
                    var l = labelsForIds[element.id];
                    if (l != '') {
                      element.label = l;
                    }
                  }
                  formObject.elements.push(element);
              }
              if (formObject.elements.length > 0) {
                  formObject.framePath = path;
                  existingList.push(JSON.stringify(formObject));
              }
          }
      }
  }

function findFormsInWindow(findLabels){
    var forms = new Array;
    findFormsRecursive(window, forms, [], findLabels);
    return forms;
}

//Fills a single element in a form
//Arguments:
//path: the frame path
//form: the name of the form the element to fill belongs to
//element: the name of the element to fill
//value: the value to insert
function fillFormElement(path, form, element, value){
    var frm = window;
    if (path === "") {
      path = [];
    } else {
      path = [path];
    }
    for(var i=0; i < path.length; ++i) frm=frm.frames[i];
    if (frm.document.forms[form] && frm.document.forms[form].elements[element]){
        frm.document.forms[form].elements[element].value=value;
    }
}
