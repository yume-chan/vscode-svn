import * as ts from "typescript";
import * as Lint from "tslint";

export class Rule extends Lint.Rules.AbstractRule {
    static metadata: Lint.IRuleMetadata = {
        ruleName: "eslint-curly",
        type: "style",
        description: "Port eslint curly to tslint",
        optionsDescription: "",
        options: null,
        typescriptOnly: false,
    }

    apply(sourceFile: ts.SourceFile): Lint.RuleFailure[] {
        return this.applyWithWalker(new Walker(sourceFile, Rule.metadata.ruleName, undefined));
    }
}

enum NodeType {
    Undefined,
    Statement,
    Block
}

class UndefinedAnalyzeResult {
    readonly type: NodeType.Undefined = NodeType.Undefined;
}

class StatementAnalyzeResult {
    readonly type: NodeType.Statement = NodeType.Statement;

    get shouldBeBlock(): boolean {
        return !this.ok;
    }

    constructor(readonly ok: boolean) { }
}

class BlockAnalyzeResult {
    readonly type: NodeType.Block = NodeType.Block;
    readonly nested: ts.Block[] = [];
    ok: boolean;

    get shouldBeBlock(): boolean {
        return this.ok;
    }

    constructor(readonly isThen: boolean) { }
}

type NodeAnalyzeResult = StatementAnalyzeResult | BlockAnalyzeResult;

class Walker extends Lint.AbstractWalker<void> {
    addFailureAtNode(node: ts.Node, failure: string, fix?: Lint.Fix): void;
    addFailureAtNode(node: ts.Node, type: string, need: boolean, fix?: Lint.Fix): void;
    addFailureAtNode(node: ts.Node, type: string, need: boolean | Lint.Fix | undefined, fix?: Lint.Fix): void {
        if (typeof need !== "boolean")
            super.addFailureAtNode(node, type, need);

        const message = `This "${type}" statement ${need ? "needs" : "doesn't need"} braces.`;
        super.addFailureAtNode(node, message, fix);
    }

    hasLeadingComments(node: ts.Node): boolean {
        return ts.getLeadingCommentRanges(this.sourceFile.text, node.pos) !== undefined;
    }

    isSingleLine(statement: ts.Node): boolean {
        const start = this.sourceFile.getLineAndCharacterOfPosition(statement.getStart());
        const end = this.sourceFile.getLineAndCharacterOfPosition(statement.getEnd());
        return start.line === end.line;
    }

    analyzeNode(node: ts.Node, isThen: boolean): NodeAnalyzeResult;
    analyzeNode(node: ts.Node | undefined, isThen: boolean): UndefinedAnalyzeResult | NodeAnalyzeResult;
    analyzeNode(node: ts.Node | undefined, isThen: boolean): UndefinedAnalyzeResult | NodeAnalyzeResult {
        if (node === undefined)
            return { type: NodeType.Undefined };

        if (ts.isBlock(node)) {
            const result = new BlockAnalyzeResult(isThen);
            this.visitBlock(node, result);
            return result;
        } else {
            const result = new BlockAnalyzeResult(isThen);
            if (this.visitSpecialNode(node, result))
                return new StatementAnalyzeResult(!result.ok);

            if (this.hasLeadingComments(node))
                return new StatementAnalyzeResult(false);

            const isSingleLine = this.isSingleLine(node);
            return new StatementAnalyzeResult(isSingleLine);
        }
    }

    visitBlock(node: ts.Block, result: BlockAnalyzeResult): void {
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

    visitIfStatement(node: ts.IfStatement, result?: BlockAnalyzeResult): void {
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
                this.addFailureAtNode(elseStatement!.getFirstToken(), "else", true);
        } else if (elseStatementState.type !== NodeType.Undefined && elseStatementState.shouldBeBlock) {
            if (thenStatementState.type != NodeType.Block)
                this.addFailureAtNode(then.getFirstToken(), "if", true);

            if (elseStatementState.type === NodeType.Statement)
                this.addFailureAtNode(elseStatement!.getFirstToken(), "else", true);
        } else {
            if (thenStatementState.type == NodeType.Block) {
                this.addFailureAtNode(then.getFirstToken(), "if", false);
                for (const item of thenStatementState.nested)
                    this.addFailureAtNode(item.getFirstToken(), "block", false);
            }

            if (elseStatementState.type == NodeType.Block) {
                this.addFailureAtNode(elseStatement!.getFirstToken(), "else", false);
                for (const item of elseStatementState.nested)
                    this.addFailureAtNode(item.getFirstToken(), "block", false);
            }
        }
    }

    visitIterationStatement(node: ts.IterationStatement, result?: BlockAnalyzeResult): void {
        let name: string;
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

    visitSpecialNode(node: ts.Node, result?: BlockAnalyzeResult): boolean {
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

    visitNode = (node: ts.Node): void => {
        if (this.visitSpecialNode(node))
            return;

        this.visitChildren(node);
    }

    visitChildren(node: ts.Node): void {
        node.forEachChild(this.visitNode);
    }

    walk(sourceFile: ts.SourceFile): void {
        this.visitChildren(sourceFile);
    }
}
