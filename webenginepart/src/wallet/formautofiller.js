function labelsForIdsInFrame(frm) {
  var labelList = frm.document.getElementsByTagName("label");
  var res = new Object;
  for (var i = 0; i < labelList.length; i++) {
    var l = labelList[i];
    if (l.htmlFor != '') {
      var obj = frm.document.getElementById(l.htmlFor);
      if (obj) {
        res[obj.id] = l.textContent;
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

function findFormsInWindow(findLabels){
    var forms = new Array;
    findFormsRecursive(window, forms, [], findLabels);
    return forms;
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
              var hasPassword = false;
              var inputList = formList[i].elements;
              if (inputList.length < 1) {
                  continue;
              }
              var formObject = new Object;
              formObject.url = url;
              formObject.name = formList[i].name;
              if (typeof(formObject.name) != 'string' || formObject.name == 0) {
                  formObject.name = String(formList[i].id);
              }
              formObject.index = String(i);
              formObject.elements = new Array;
              for (var j = 0; j < inputList.length; ++j) {
                  var element = objectFromElement(inputList[j]);
                  if (element == null) {
                    continue;
                  }
                  if (element.isPassword) {
                    hasPassword = true;
                  }
                  if (findLabels && element.id) {
                    var l = labelsForIds[element.id];
                    if (l != '') {
                      element.label = l;
                    }
                  }
                  formObject.elements.push(element);
              }
              if (formObject.elements.length > 0) {
                  if (hasPassword) {
                    updateMaybeLogins(formObject.elements);
                  }
                  formObject.framePath = path;
                  existingList.push(JSON.stringify(formObject));
              }
          }
      }
  }

//Creates an object with information about the given input element
//
// This function returns null in the following cases:
// - the element is not an input element
// - the element has neither a name nor an id
//
//The returned object has the following entries:
//- id: the id of the element
//- name: the name of the element or its id if the name is undefined
//- type: the type of the input element
//- value: the value of the element
//- readonly: whether the element is read-only or not
//- disabled: whether the element is disabled or not
//- autocompleteAllowed: whether autocomplete is allowed for the element
function objectFromElement(el) {
    if (el.type != 'text' && el.type != 'email' && el.type != 'password') {
      return null;
    }
    var obj = new Object;
    obj.id = String(el.id);
    obj.name = el.name;
    var hasName = true;
    if (typeof(obj.name) != 'string' || obj.name.length == 0) {
        obj.name = obj.id;
        hasName = false;
    }
    if (obj.name.length == 0) {
        return null;
    }
    obj.type = el.type;
    obj.value = String(el.value);
    obj.readonly = Boolean(el.readOnly);
    obj.disabled = Boolean(el.disabled);
    obj.isPassword = el.type == 'password';
    obj.autocompleteAllowed = el.autocomplete != 'off' || el.type == 'password';
    if (!obj.autocompleteAllowed) {
      obj.maybeLogin = canBeLoginIdentifier(obj.id.toLowerCase());
      if (hasName && !obj.maybeLogin) {
        obj.maybeLogin = canBeLoginIdentifier(obj.name.toLowerCase());
      }
    }
    return obj;
}

// Whether _string_ can be an identifier (name or id) of a input field meant to
// enter a user name.
// Currently the heuristics used to decide this is simply to check whether _string_
// contains the substring "login" or "username". The match is case sensitive
function canBeLoginIdentifier(string) {
  return string.includes("login") || string.includes("username");
}

function updateMaybeLogins(elements) {
  for (var i = 0; i < elements.length; ++i) {
    var e = elements[i];
    if (e.maybeLogin) {
      e.autocompleteAllowed = true;
    }
  }
}

//Finds an element given its name and the containing form's frame, name and index
//It first tries to find the form based on its name. If it doesn't find it or if
//the form doesn't contain an element called elementName, it attempts again to find
//the form using its index. This is needed in case there are several forms with the
//same name.
//If all attempt fails, null is returned
function findElementInForm(frame, formName, formIndex, elementName) {
  let form = frame.document.forms[formName];
  let el = form ? form.elements[elementName] : null;
  if (!el) {
    form = frame.document.forms[formIndex];
    if (form) el = form.elements[elementName];
  }
  return el;
}

//Fills a single element in a form
//Arguments:
//path: the frame path
//formName: the name of the form the element to fill belongs to
//index: the index of the form
//elementName: the name of the element to fill
//value: the value to insert
//This function works as follows:
//- it looks for the frame corresponding to path
//- it uses findElementInForm to attempt to find the element
//- if no element is found, it returns false
//- it sets the value of the element to value
//- it attempts to simulate real key presses by sending the blur, change and input events
//
//Note: it's important to return a value, otherwise the C++ callback will think
//it has been called from the page's destructor
function fillFormElement(path, formName, index, elementName, value){
  var frm = window;
  path = path === "" ? [] : [path];
  for (var i=0; i < path.length; ++i) frm=frm.frames[i];

  let el = findElementInForm(frm, formName, index, elementName);
  if (!el) {
    return false;
  }
  el.value=value;
  //Attempt to simulate the field being filled by the user by sending the
  //appropriate events. I'm not sure whether these are the only ones needed
  const options = {bubbles: true, cancelable: false, composed: true};
  el.dispatchEvent(new Event("blur", options));
  el.dispatchEvent(new Event("change", options));
  el.dispatchEvent(new Event("input", options));

  return true;
}
