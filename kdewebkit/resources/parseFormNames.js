(function (){
    var forms = new Array;
    for (var i = 0; i < document.forms.length; ++i) {
        var form = document.forms[i];
        if (form.method.toLowerCase() == "post") {
            var formObject = new Object;
            formObject.name = form.name;
            formObject.index = i.toString();
            forms.push(formObject);
        }
    }
    return forms;
}())
