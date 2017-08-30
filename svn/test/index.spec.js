const chai = require("chai");
const chaiAsPromised = require("chai-as-promised");

chai.use(chaiAsPromised);
const { expect } = chai;

describe("svn", function() {
    let svn;

    it("should load", function() {
        expect(svn = require("..")).to.exist;
    });

    describe(".version", function() {
        it("should exist", function() {
            expect(svn).to.have.property("version");
        });

        it("should be non-configurable", function() {
            expect(svn).to.have.ownPropertyDescriptor("version").that.has.property("configurable", false);
        });

        it("should be non-writable", function() {
            expect(svn).to.have.ownPropertyDescriptor("version").that.has.property("writable", false);
        });

        it("should be an object", function() {
            expect(svn.version).to.be.an("object");
        });

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
        it("should exist", function() {
            expect(svn).to.have.property("SvnError");
        });

        it("should be non-configurable", function() {
            expect(svn).to.have.ownPropertyDescriptor("SvnError").that.has.property("configurable", false);
        });

        it("should be non-writable", function() {
            expect(svn).to.have.ownPropertyDescriptor("SvnError").that.has.property("writable", false);
        });

        it("should be a function", function() {
            expect(svn.SvnError).to.be.an("function");
        });

        it("should throw if not be invoked with new", function() {
            expect(svn.SvnError).to.throw();
        });

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
        it("should exist", function() {
            expect(svn).to.have.property("Client");
        });

        it("should be non-configurable", function() {
            expect(svn).to.have.ownPropertyDescriptor("Client").that.has.property("configurable", false);
        });

        it("should be non-writable", function() {
            expect(svn).to.have.ownPropertyDescriptor("Client").that.has.property("writable", false);
        });

        it("should be a function", function() {
            expect(svn.Client).to.be.an("function");
        });

        it("should have `Kind`", function() {
            expect(svn.Client).to.have.property("Kind").that.is.an("object");

            for (const key in svn.Client.Kind)
                expect(svn.Client.Kind[key], key).to.be.a("number");
        });

        it("should have `StatusKind`", function() {
            expect(svn.Client).to.have.property("StatusKind").that.is.an("object");

            for (const key in svn.Client.StatusKind)
                expect(svn.Client.StatusKind[key], key).to.be.a("number");
        });

        it("should throw if not be invoked with new", function() {
            expect(svn.Client).to.throw();
        });

        let client;

        it("should be a constructor", function() {
            expect(client = new svn.Client()).to.be.an.instanceof(svn.Client);
        });

        describe("#status", function() {
            it("should exist", function() {
                expect(client.status).to.be.a("function");
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

        describe("#cat", function() {
            it("should exist", function() {
                expect(client.cat).to.be.a("function");
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
    });
});
