const svn = require("..");
const client = new svn.Client();
client.cat("C:/Users/Simon/Desktop/www/bdxkv4/index.html").then(value => value.toString("utf8")).then(value => console.log(value), error => console.error(error));

(function wait() {
    setTimeout(wait, 1000);
})();
