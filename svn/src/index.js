const svn = require("..");

console.log(`Path: ${svn.path}`);

(async function() {
    try {
        const client = new svn.Client({
            getSimpleCredential(realm, username) {
                console.log("getSimpleCredential");
                console.log(`realm: ${realm}`);
                console.log(`username: ${username}`);
                return Promise.resolve({
                    username: "ad_cxm",
                    password: "cxm123",
                });
            }
        });

        console.log(await client.status("C:/Users/Simon/Desktop/www/test/trunk"));

        // console.log(await client.commit("C:/Users/Simon/Desktop/www/test/trunk", "Test"));
        // console.log(await client.update("C:/Users/Simon/Desktop/www/test/trunk"));

        {
            const file = "C:/Users/Simon/Desktop/www/webchat/datasvr/invest/getContent.cfg";
            const buffer = await client.cat(file);
            const content = buffer.toString("utf8");
            console.log(content);
        }

        {
            const file = "C:/Users/Simon/Desktop/www/test/trunk";
            const buffer = await client.cat(file);
            const content = buffer.toString("utf8");
            console.log(content);
        }
    } catch (e) {
        console.log(e);
    }

    process.exit();
})();

(function wait() {
    setTimeout(wait, 1000);
})();
