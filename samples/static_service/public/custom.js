function PostJsonForm(event, on_post_response) {
    event.preventDefault();

    // event.target — HTML-element of the form
    const formData = new FormData(event.target);

    var obj = {};
    formData.forEach((value, key) => obj[key] = value);
    
    fetch('/v1/messages', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify(obj),
    })
    .then(response => response.json())
    .then(on_post_response)
    .catch(error => {
        alert("Error while retrieving initial data from backend: " + error);
    });
}

function GetMessages(on_messages_response) {
    fetch('/v1/messages', {
        method: 'GET',
        headers: {'Content-Type': 'application/json'},
    })
    .then(response => response.json())
    .then(on_messages_response)
    .catch(error => {
        alert("Error while retrieving initial data from backend: " + error);
    });
}
