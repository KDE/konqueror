(function (){
    var forms = new Array;
    for (var i = 0; i < document.forms.length; ++i) {
        var form = document.forms[i];
        if (form.method.toLowerCase() == "post") {
            var formObject = new Object;
            formObject.name = form.name;
            formObject.index = i.toString();
            var elements = new Array;
            var passwordFieldCount = 0;
            for (var j = 0; j < form.elements.length; ++j) {
                var e = form.elements[j];
                var type = e.type.toLowerCase();
                if ((type == "password" || type == "text") && e.value.length > 0) {
                    var element = new Object;
                    element.name = e.name;
                    element.value = e.value;
                    element.type = e.type;
                    element.autocomplete = e.attributes.getNamedItem("autocomplete");
                    if (element.autocomplete != null)
                        element.autocomplete = element.autocomplete.value;
                    elements.push(element);
                    if (type == "password")
                        passwordFieldCount++;
                }
            }

            if (elements.length > 0 && passwordFieldCount > 0) {
                formObject.elements = elements;
                forms.push(formObject);
            }
        }
    }
    return forms;
}())
