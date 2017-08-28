const svn = require("..");
const client = new svn.Client();
client.status("").then(value => console.log(value), error => console.error(error));

(function wait() {
    setTimeout(wait, 1000);
})();
