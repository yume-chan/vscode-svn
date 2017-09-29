const chai = require("chai");
const chaiAsPromised = require("chai-as-promised");

chai.use(chaiAsPromised);
const { expect } = chai;

let svn;
let client;

function describeProperty(object, key, type) {
    it("should exist", function() {
        expect(eval(object)).to.have.property(key);
    });

    it("should be a " + type, function() {
        expect(eval(object)).to.have.property(key).that.is.a(type);
    });

    it("should be non-configurable", function() {
        expect(eval(object)).to.have.ownPropertyDescriptor(key).that.has.property("configurable", false);
    });

    it("should be non-writable", function() {
        expect(eval(object)).to.have.ownPropertyDescriptor(key).that.has.property("writable", false);
    });
}

function describeFunction(object, key, length) {
    it("should exist", function() {
        expect(eval(object)).to.have.property(key);
    });

    it("should be a function", function() {
        expect(eval(object)).to.have.property(key).that.is.a("function");
    });

    it("should have length equals to " + length, function() {
        expect(eval(object)).to.have.property(key).that.has.property("length", length);
    });
}

function describeConstrcutor(object, key, length) {
    describeFunction(object, key, length);

    it("should throw if not be invoked with new", function() {
        expect(eval(object)).to.have.property(key).that.throws();
    });
}

const values = {
    "string": ["", "test"],
    "number": [-1, 0, 1, Infinity, NaN],
    "boolean": [false, true],
    "array": [[], ["test"]],
    "object": [{}, { test: "test" }],
    "other": [undefined, null],
}

/**
 * @param {string} object
 * @param {string} name
 * @param {string | string[]} types
 * @param {any[]} args
 */
function describeArgument(object, name, types, ...args) {
    if (typeof types === "string")
        types = [types];

    it(`should have argument ${args.length} as a ${types.join(" or ")}`, function() {
        for (const key in values) {
            if (!types.includes(key))
                continue;

            for (const value of values[key])
                expect(eval(`${object}.${name}`).call(eval(object), ...args, value)).to.be.rejected;
        }
    });
}

describe("svn", function() {
    it("should load", function() {
        expect(svn = require("..")).to.exist;
    });

    describe(".version", function() {
        describeProperty("svn", "version", "object");

        it("should have a number `major`", function() {
            expect(svn.version).to.have.property("major").that.is.a("number");
        });

        it("should have a number `minor`", function() {
            expect(svn.version).to.have.property("minor").that.is.a("number");
        });

        it("should have a number `patch`", function() {
            expect(svn.version).to.have.property("patch").that.is.a("number");
        });
    });

    describe(".SvnError", function() {
        describeConstrcutor("svn", "SvnError", 2);

        let error;

        it("should be a constructor", function() {
            expect(error = new svn.SvnError(0, "")).to.be.an.instanceof(svn.SvnError);
        });

        it("should extend Error", function() {
            expect(error).to.be.an.instanceof(Error);
        });

        it("should have name", function() {
            expect(error).to.have.property("name", "SvnError");
        });

        it("should have stack", function() {
            expect(error).to.have.property("stack");
        });

        it("should have code", function() {
            expect(new svn.SvnError(42, "test")).to.have.property("code", 42);
        });

        it("should have message", function() {
            expect(new svn.SvnError(0, "test")).to.have.property("message", "test");
        });
    });

    describe(".Client", function() {
        describeConstrcutor("svn", "Client", 0);

        describe(".Kind", function() {
            describeProperty("svn.Client", "Kind", "object");

            it("should be an enum", function() {
                for (const key in svn.Client.Kind) {
                    const index = parseInt(key);
                    if (isNaN(index)) {
                        expect(svn.Client.Kind).to.have.property(key).that.is.a("number");
                        expect(svn.Client.Kind).to.have.property(svn.Client.Kind[key], key);
                    } else {
                        expect(svn.Client.Kind).to.have.property(key).that.is.a("string");
                        expect(svn.Client.Kind).to.have.property(svn.Client.Kind[key], index);
                    }
                }
            });
        });

        describe(".StatusKind", function() {
            describeProperty("svn.Client", "StatusKind", "object");

            it("should be an enum", function() {
                for (const key in svn.Client.StatusKind) {
                    const index = parseInt(key);
                    if (isNaN(index)) {
                        expect(svn.Client.StatusKind).to.have.property(key).that.is.a("number");
                        expect(svn.Client.StatusKind).to.have.property(svn.Client.StatusKind[key], key);
                    } else {
                        expect(svn.Client.StatusKind).to.have.property(key).that.is.a("string");
                        expect(svn.Client.StatusKind).to.have.property(svn.Client.StatusKind[key], index);
                    }
                }
            });
        });


        it("should be a constructor", function() {
            expect(client = new svn.Client()).to.be.an.instanceof(svn.Client);
        });

        describe("#add", function() {
            describeFunction("svn.Client.prototype", "add", 1);

            it("should exist on instance", function() {
                expect(client).to.have.property("add", svn.Client.prototype.add);
            });

            describeArgument("client", "add", "string");
        });

        describe("#cat", function() {
            describeFunction("svn.Client.prototype", "cat", 1);

            it("should exist on instance", function() {
                expect(client).to.have.property("cat", svn.Client.prototype.cat);
            });

            describeArgument("client", "cat", "string");
        });

        describe("#changelistAdd", function() {
            describeFunction("svn.Client.prototype", "changelistAdd", 2);

            it("should exist on instance", function() {
                expect(client).to.have.property("changelistAdd", svn.Client.prototype.changelistAdd);
            });

            describeArgument("client", "changelistAdd", "string | array");
            describeArgument("client", "changelistAdd", "string", "test");
        });

        describe("#changelistRemove", function() {
            describeFunction("svn.Client.prototype", "changelistRemove", 1);

            it("should exist on instance", function() {
                expect(client).to.have.property("changelistRemove", svn.Client.prototype.changelistRemove);
            });

            describeArgument("client", "changelistRemove", "string | array");
        });

        describe("#checkout", function() {
            describeFunction("svn.Client.prototype", "checkout", 2);

            it("should exist on instance", function() {
                expect(client).to.have.property("checkout", svn.Client.prototype.checkout);
            });

            describeArgument("client", "checkout", "string");
            describeArgument("client", "checkout", "string", "test");
        });

        describe("#commit", function() {
            describeFunction("svn.Client.prototype", "commit", 2);

            it("should exist on instance", function() {
                expect(client).to.have.property("commit", svn.Client.prototype.commit);
            });

            describeArgument("client", "commit", "string | array");
            describeArgument("client", "commit", "string", "test");
        });

        describe("#delete", function() {
            describeFunction("svn.Client.prototype", "delete", 1);

            it("should exist on instance", function() {
                expect(client).to.have.property("delete", svn.Client.prototype.delete);
            });

            describeArgument("client", "delete", "string | array");
        });

        describe("#info", function() {
            describeFunction("svn.Client.prototype", "info", 1);

            it("should exist on instance", function() {
                expect(client).to.have.property("info", svn.Client.prototype.info);
            });

            describeArgument("client", "info", "string");
        });

        describe("#status", function() {
            describeFunction("svn.Client.prototype", "status", 1);

            it("should exist on instance", function() {
                expect(client).to.have.property("status", svn.Client.prototype.status);
            });

            describeArgument("client", "status", "string");
        });
    });
});
