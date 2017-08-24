"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const ts = require("typescript");
const Lint = require("tslint");
class Rule extends Lint.Rules.AbstractRule {
    apply(sourceFile) {
        return this.applyWithWalker(new Walker(sourceFile, Rule.metadata.ruleName, undefined));
    }
}
Rule.metadata = {
    ruleName: "eslint-curly",
    type: "style",
    description: "Port eslint curly to tslint",
    optionsDescription: "",
    options: null,
    typescriptOnly: false,
};
exports.Rule = Rule;
var NodeType;
(function (NodeType) {
    NodeType[NodeType["Undefined"] = 0] = "Undefined";
    NodeType[NodeType["Statement"] = 1] = "Statement";
    NodeType[NodeType["Block"] = 2] = "Block";
})(NodeType || (NodeType = {}));
class UndefinedAnalyzeResult {
    constructor() {
        this.type = NodeType.Undefined;
    }
}
class StatementAnalyzeResult {
    constructor(ok) {
        this.ok = ok;
        this.type = NodeType.Statement;
    }
    get shouldBeBlock() {
        return !this.ok;
    }
}
class BlockAnalyzeResult {
    constructor(isThen) {
        this.isThen = isThen;
        this.type = NodeType.Block;
        this.nested = [];
    }
    get shouldBeBlock() {
        return this.ok;
    }
}
class Walker extends Lint.AbstractWalker {
    constructor() {
        super(...arguments);
        this.visitNode = (node) => {
            if (this.visitSpecialNode(node))
                return;
            this.visitChildren(node);
        };
    }
    addFailureAtNode(node, type, need, fix) {
        if (typeof need !== "boolean")
            super.addFailureAtNode(node, type, need);
        const message = `This "${type}" statement ${need ? "needs" : "doesn't need"} braces.`;
        super.addFailureAtNode(node, message, fix);
    }
    hasLeadingComments(node) {
        return ts.getLeadingCommentRanges(this.sourceFile.text, node.pos) !== undefined;
    }
    isSingleLine(statement) {
        const start = this.sourceFile.getLineAndCharacterOfPosition(statement.getStart());
        const end = this.sourceFile.getLineAndCharacterOfPosition(statement.getEnd());
        return start.line === end.line;
    }
    analyzeNode(node, isThen) {
        if (node === undefined)
            return { type: NodeType.Undefined };
        if (ts.isBlock(node)) {
            const result = new BlockAnalyzeResult(isThen);
            this.visitBlock(node, result);
            return result;
        }
        else {
            const result = new BlockAnalyzeResult(isThen);
            if (this.visitSpecialNode(node, result))
                return new StatementAnalyzeResult(!result.ok);
            if (this.hasLeadingComments(node))
                return new StatementAnalyzeResult(false);
            const isSingleLine = this.isSingleLine(node);
            return new StatementAnalyzeResult(isSingleLine);
        }
    }
    visitBlock(node, result) {
        if (node.statements.length != 1) {
            result.ok = true;
            this.visitChildren(node);
            return;
        }
        const statement = node.statements[0];
        if (!result.ok)
            result.ok = this.hasLeadingComments(statement);
        if (this.visitSpecialNode(statement, result))
            return;
        if (ts.isBlock(statement)) {
            result.nested.push(statement);
            this.visitBlock(statement, result);
            return;
        }
        if (!result.ok)
            result.ok = !this.isSingleLine(statement);
    }
    visitIfStatement(node, result) {
        const elseStatement = node.elseStatement;
        const elseStatementState = this.analyzeNode(elseStatement, false);
        const then = node.thenStatement;
        const thenStatementState = this.analyzeNode(then, elseStatement !== undefined);
        if (result !== undefined && !result.ok) {
            if (result.isThen && elseStatement === undefined)
                result.ok = true;
            else
                result.ok = elseStatement !== undefined || thenStatementState.shouldBeBlock;
        }
        if (thenStatementState.shouldBeBlock) {
            if (thenStatementState.type != NodeType.Block)
                this.addFailureAtNode(then.getFirstToken(), "if", true);
            if (elseStatementState.type === NodeType.Statement)
                this.addFailureAtNode(elseStatement.getFirstToken(), "else", true);
        }
        else if (elseStatementState.type !== NodeType.Undefined && elseStatementState.shouldBeBlock) {
            if (thenStatementState.type != NodeType.Block)
                this.addFailureAtNode(then.getFirstToken(), "if", true);
            if (elseStatementState.type === NodeType.Statement)
                this.addFailureAtNode(elseStatement.getFirstToken(), "else", true);
        }
        else {
            if (thenStatementState.type == NodeType.Block) {
                this.addFailureAtNode(then.getFirstToken(), "if", false);
                for (const item of thenStatementState.nested)
                    this.addFailureAtNode(item.getFirstToken(), "block", false);
            }
            if (elseStatementState.type == NodeType.Block) {
                this.addFailureAtNode(elseStatement.getFirstToken(), "else", false);
                for (const item of elseStatementState.nested)
                    this.addFailureAtNode(item.getFirstToken(), "block", false);
            }
        }
    }
    visitIterationStatement(node, result) {
        let name;
        switch (node.kind) {
            case ts.SyntaxKind.ForStatement:
                name = "for";
                break;
            case ts.SyntaxKind.ForInStatement:
                name = "for...in";
                break;
            case ts.SyntaxKind.ForOfStatement:
                name = "for...of";
                break;
            case ts.SyntaxKind.WhileStatement:
                name = "while";
                break;
            case ts.SyntaxKind.DoStatement:
                name = "do";
                break;
            default:
                name = ts.SyntaxKind[node.kind];
                break;
        }
        const statement = node.statement;
        const statementState = this.analyzeNode(statement, result !== undefined && result.isThen);
        if (result !== undefined && !result.ok)
            result.ok = statementState.shouldBeBlock;
        if (!statementState.ok) {
            if (statementState.type === NodeType.Statement)
                this.addFailureAtNode(node.statement.getFirstToken(), name, true);
            if (statementState.type === NodeType.Block)
                this.addFailureAtNode(statement.getFirstToken(), name, false);
        }
    }
    visitSpecialNode(node, result) {
        if (ts.isIfStatement(node)) {
            this.visitIfStatement(node, result);
            return true;
        }
        if (ts.isIterationStatement(node, false)) {
            this.visitIterationStatement(node, result);
            return true;
        }
        return false;
    }
    visitChildren(node) {
        node.forEachChild(this.visitNode);
    }
    walk(sourceFile) {
        this.visitChildren(sourceFile);
    }
}
//# sourceMappingURL=eslintCurlyRule.js.map