const { expect } = require("chai");

const svn = require("..");

describe("Version", function() {
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

describe("Client", function() {
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

    it("should have `status` member method", function() {
        expect(new svn.Client().status).to.be.a("function");
    });

    it("should have `cat` member method", function() {
        expect(new svn.Client().cat).to.be.a("function");
    });
});
