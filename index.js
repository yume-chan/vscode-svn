const svn = require("./build/Release/svn.node");

console.log(svn.version());
console.log(svn.Client);

const client = new svn.Client();
console.log(client);
const status = client.status("C:/Sandbox/Simon/DefaultBox/drive/D/svn");
console.log(status);
