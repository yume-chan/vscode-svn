const { expect } = require("chai");

if (typeof describe === "undefined")
    global.describe = (description, callback) => callback();

if (typeof it === "undefined")
    global.it = (expectation, callback) => callback();

describe("svn", function() {
    let svn;

    it("should load", function() {
        svn = require("..");
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

        it("should throw if not be invoked with new", function() {
            expect(svn.Client).to.throw();
        });

        it("should be a constructor", function() {
            expect(new svn.Client()).to.be.an.instanceof(svn.Client);
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

        describe("#status", function() {
            it("should exist", function() {
                expect(new svn.Client().status).to.be.a("function");
            });

            it("should take a string", function() {
                expect(() => { new svn.Client().status(); }).to.throw();
                expect(() => { new svn.Client().status(undefined); }).to.throw();
                expect(() => { new svn.Client().status(1); }).to.throw();
                expect(() => { new svn.Client().status(true); }).to.throw();
                expect(() => { new svn.Client().status(["path"]); }).to.throw();
                expect(() => { new svn.Client().status({ "path": "" }); }).to.throw();
            });
        });

        describe("Client#cat", function() {
            it("should exist", function() {
                expect(new svn.Client().cat).to.be.a("function");
            });

            it("should take a string", function() {
                expect(() => { new svn.Client().cat(); }).to.throw();
                expect(() => { new svn.Client().cat(undefined); }).to.throw();
                expect(() => { new svn.Client().cat(1); }).to.throw();
                expect(() => { new svn.Client().cat(true); }).to.throw();
                expect(() => { new svn.Client().cat(["path"]); }).to.throw();
                expect(() => { new svn.Client().cat({ "path": "" }); }).to.throw();
            });
        });
    });
});
