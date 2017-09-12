const svn = require("..");

console.log(`Path: ${svn.path}`);

(async function() {
    try {
        const client = new svn.Client();
        // console.log(await client.commit("C:/Users/Simon/Desktop/www/test/trunk", "Test"));
        console.log(await client.update("C:/Users/Simon/Desktop/www/test/trunk"));
    } catch (e) {
        console.log(e);
    }

    process.exit();
})();

(function wait() {
    setTimeout(wait, 1000);
})();
