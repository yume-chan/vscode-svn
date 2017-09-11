const chai = require("chai");
const chaiAsPromised = require("chai-as-promised");

chai.use(chaiAsPromised);
const { expect } = chai;

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
    describeProperty(object, key, "function");

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

let svn;

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

        let client;

        it("should be a constructor", function() {
            expect(client = new svn.Client()).to.be.an.instanceof(svn.Client);
        });

        describe("#add", function() {
            describeFunction("svn.Client.prototype", "add", 1);

            it("should exist on instance", function() {
                expect(client).to.have.property("add", svn.Client.prototype.add);
            });

            it("should take a string", function() {
                expect(client.add()).to.be.rejected;
                expect(client.add(undefined)).to.be.rejected;
                expect(client.add(1)).to.be.rejected;
                expect(client.add(true)).to.be.rejected;
                expect(client.add(["path"])).to.be.rejected;
                expect(client.add({ "path": "" })).to.be.rejected;
            });
        });

        describe("#cat", function() {
            describeFunction("svn.Client.prototype", "cat", 1);

            it("should exist on instance", function() {
                expect(client).to.have.property("cat", svn.Client.prototype.cat);
            });

            it("should take a string", function() {
                expect(client.cat()).to.be.rejected;
                expect(client.cat(undefined)).to.be.rejected;
                expect(client.cat(1)).to.be.rejected;
                expect(client.cat(true)).to.be.rejected;
                expect(client.cat(["path"])).to.be.rejected;
                expect(client.cat({ "path": "" })).to.be.rejected;
            });
        });

        describe("#checkout", function() {
            describeFunction("svn.Client.prototype", "checkout", 2);

            it("should exist on instance", function() {
                expect(client).to.have.property("checkout", svn.Client.prototype.checkout);
            });
        });

        describe("#status", function() {
            describeFunction("svn.Client.prototype", "status", 1);

            it("should exist on instance", function() {
                expect(client).to.have.property("status", svn.Client.prototype.status);
            });

            it("should take a string", function() {
                expect(client.status()).to.be.rejected;
                expect(client.status(undefined)).to.be.rejected;
                expect(client.status(1)).to.be.rejected;
                expect(client.status(true)).to.be.rejected;
                expect(client.status(["path"])).to.be.rejected;
                expect(client.status({ "path": "" })).to.be.rejected;
            });
        });
    });
});
