/**
 * Finds the label for each element with an id in a frame
 *
 * It looks for all label elements in the frame. For each label element, it then
 * looks for the element specified in its htmlFor attribute.
 *
 * @param {frame} frm the frame where to look for the labels
 * @return {object} an object having the id of the element as keys and the label text as values
 */
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

/**
 * Recursively finds all forms with fillable elements in a frame
 *
 * If the frame doesn't contain any forms but it contains an input element with the password type,
 * it returns a fake form containing input elements outside forms. See findFormsInFrame() for more
 * information.
 *
 * @param {frame} wnd the frame where to look for forms
 * @param {array} path a list of indexes which uniquely identify the frame, according to their index in the parent frame's frames property
 * @param {boolean} findLabels whether or not form information should include the labels for each element
 * @return {array} a list of objects, each representing a form
 * @see findFormsInFrame()
 */
function findFormsRecursive(wnd, path, findLabels) {
    let result = findFormsInFrame(wnd, path, findLabels);
    //We can't use map because wnd.frames isn't a real array and there's no simple
    //way to convert it into one
    const frameList = wnd.frames;
    for(let i = 0; i < frameList.length; ++i) {
        const newPath = path.concat(i);
        result = result.concat(findFormsRecursive(frameList[i], newPath, findLabels));
    }
    return result;
}

/**
 * Recursively find all forms with fillable elements
 *
 * It's a convenience wrapper around findFormsRecursive() which automatically
 * uses the window as frame.
 *
 * @param {boolean} findLabels whether or not form information should include the labels for each element
 * @return {array} a list of objects, each representing a form
 */
function findFormsInWindow(findLabels){
    return findFormsRecursive(window, [], findLabels);
}

/**
 * Finds all forms with fillable elements in the given frame
 *
 * Fillable elements are input elements with the text, email or password type.
 *
 * If no forms with fillable elements are found, but there are input elements with
 * the password type outside forms, then input elements outside forms are considered
 * to be a part of a fake form with no name and index -1.
 *
 * @param {frame} frm the frame where to look for forms
 * @param {array} path a list of indexes which uniquely identify the frame, according to their index in the parent frame's frames property
 * @param {boolean} findLabels whether or not form information should include the labels for each element
 * @return {array} an array of objects, each representing a form with fillable fields or an array with a single object representing a fake
 * form. Each form object has the characteristics described in formAsObject().
 * @see formAsObject()
 */
function findFormsInFrame(frm, path, findLabels){
    const url = frm.location;
    let formList;
    try{ formList = frm.document.forms; }
    catch(e){
        return [];
    }

    var labelsForIds;
    if (findLabels) {
        labelsForIds = labelsForIdsInFrame(frm);
    }
    const formObjects = Array.from(formList, (f, i) => formAsObject(f, url, i, path, Array.from(f.elements), labelsForIds)).filter(f => f != null);

    if (!formObjects.some(f => f.hasPassword)) {
        selectFormlessInputs = (el) => !el.form && (el.type == 'text' || el.type == 'email' || el.type == 'password')
        const formlessInputs = Array.from(frm.document.getElementsByTagName("input")).filter(selectFormlessInputs);
        if (formlessInputs.some(f => f.type == 'password')) {
          formObjects.push(formAsObject(null, url, -1, path, formlessInputs, labelsForIds));
        }
    }

    return formObjects;
}

/**
 * Creates an object representing a form with fillable elements
 *
 * The object has the following attributes:
 * - url: the URL of the form
 * - name: the name of the form. It's empty if the object represents a fake form. If the form
 * has an id but not a name, the id is used instead of the name
 * - index: a string representation of the index parameter. It represents the number -1 if the object represents a fake form
 * - elements: an array of fillable elements in the form. Each element is an object as described in
 * objectFromElement()
 * - path an array of indexes which uniquely identify the frame the form belongs to, according to their index in the parent frame's frames property
 *
 * @param {node} form the form
 * @param {object} url the URL of the form
 * @param {number} index the index of the form. It must be -1 for a fake form
 * @param {array} elements an array of elements in the form
 * @param {object} labels the object retured by labelsForIds() for the frame containing the form
 * @return {string} the JSON representation of the form object or null if elements is empty
 */
function formAsObject(form, url, index, path, elements, labels) {
    if (!elements || elements.length < 1) {
        return null;
    }
    const formObject = new Object;
    formObject.url = url;
    formObject.name = form ? form.name : '';
    if (form && (typeof(formObject.name) != 'string' || formObject.name == 0)) {
        formObject.name = String(form.id);
    }
    formObject.index = String(index);
    const elementObjects = elements.map(e => objectFromElement(e, labels)).filter(e => e != null);
    formObject.hasPassword = elementObjects.some(e => e.type == 'password');
    if (formObject.hasPassword) {
      updateMaybeLogins(elementObjects);
    }
    formObject.elements = elementObjects;
    formObject.framePath = path;
    return JSON.stringify(formObject);
}

/**
 * Creates an object with information about the given input element
 *
 * The returned object has the following attributes:
 * - id: the id of the element
 * - name: the name of the element or its id if the name is undefined
 * - type: the type of the input element
 * - value: the value of the element
 * - readonly: whether the element is read-only or not
 * - disabled: whether the element is disabled or not
 * - autocompleteAllowed: whether autocomplete is allowed for the element
 *
 * @param {node} el the element
 * @return an object representing the element or null if the element isn't an input
 * element with type text, email or password or if element doesn't have either a
 * name or an id
 */
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

/**
 * Determines whether a string can be the identifier of a input field meant to
 * enter a user name
 *
 * Currently the heuristics used to decide this is simply to check whether string
 * contains the substring "login" or "username". The match is case sensitive.
 *
 * @param {string} string the string to check
 * @return {boolean} true if, according to the heuristics above, the string can
 * be the name or the id of an element where the user is supposed to enter the login
 * name and false otherwise
*/
function canBeLoginIdentifier(string) {
  return string.includes("login") || string.includes("username");
}

/**
 * Sets the autocompleteAllowed attribute of the given objects to true
 *
 * @param {array} elements an array of objects
 */
function updateMaybeLogins(elements) {
  for (var i = 0; i < elements.length; ++i) {
    var e = elements[i];
    if (e.maybeLogin) {
      e.autocompleteAllowed = true;
    }
  }
}

/**
 * Finds an element given its name and the containing form's frame, name and index
 *
 * Since there may be multiple forms with the same name, finding the element requires
 * care: the element is first searched in the form called formName and, if the search
 * fails, in the form at position index in the frame. If this search also fails,
 * the function gives up
 *
 * If name is empty and index is negative, it means that the form is a fake one so
 * we look for an element called elementName in the whole frame.
 *
 * @param {node} frame the frame where to look for the element
 * @param {string} formName the name of the form which contains the element. It can be empty
 * @param {number} formIndex the index of the form containing the element. If negative, it means the form is fake
 * @param {string} elementName the name of the element to find
 * @return {node} the element corresponding to the given criteria or null if no such
 * element can be found.
 */
function findFillableElement(frame, formName, formIndex, elementName) {
    if (formName.length > 0 || formIndex > -1) {
        let form = frame.document.forms[formName];
        let el = form ? form.elements[elementName] : null;
        if (!el) {
          form = frame.document.forms[formIndex];
          if (form) el = form.elements[elementName];
        }
        return el;
    } else {
      return frame.document.getElementsByName(elementName)[0];
    }
}

/**
 * Fills a single element in a form or document
 *
 * This function works as follows:
 * - it looks for the frame corresponding to path
 * - it uses findFillableElement() to attempt to find the element
 * - if no element is found, it returns false
 * - it sets the value of the element to the given value
 * - it attempts to simulate real key presses by sending the blur, change and input events
 *
 * @param {array} path a list of indexes which uniquely identify the frame containing the element, according to their index in the parent frame's frames property
 * @param {string} formName the name of the form the element belongs to or an empty string if the element isn't in a form
 * @param {number} index the index of the form the element belongs to or a negative number if the element isn't in a form
 * @param {string} elementName the name of the element to fill
 * @param {string} value the value to insert in the element
 * @return {boolean} true if filling the element succeeded and false otherwise
 */
function fillFormElement(path, formName, index, elementName, value){
  var frm = window;
  path = path === "" ? [] : [path];
  for (var i=0; i < path.length; ++i) frm=frm.frames[i];

  let el = findFillableElement(frm, formName, index, elementName);
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
