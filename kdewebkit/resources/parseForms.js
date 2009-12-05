(function (){
    var forms = new Array;
    for (var i = 0; i < document.forms.length; ++i) {
        var form = document.forms[i];
        if (form.method.toLowerCase() == "post") {
            var formObject = new Object;
            formObject.name = form.name;
            formObject.index = i.toString();
            var elements = new Array;
            for (var j = 0; j < form.elements.length; ++j) {
                var e = form.elements[j];
                if ((e.type.toLowerCase() == "password" || e.type.toLowerCase() == "text") && e.value.length > 0) {
                    var element = new Object;
                    element.name = e.name;
                    element.value = e.value;
                    element.type = e.type;
                    element.autocomplete = e.attributes.getNamedItem("autocomplete");
                    if (element.autocomplete != null)
                        element.autocomplete = element.autocomplete.value;
                    elements.push(element);
                }
            }

            if (elements.length > 0) {
                formObject.elements = elements;
                forms.push(formObject);
            }
        }
    }
    return forms;
}())
