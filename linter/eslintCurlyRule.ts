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

    constructor() { }
}

type NodeAnalyzeResult = StatementAnalyzeResult | BlockAnalyzeResult;

interface Branch {
    keyword: string;
    keywordRange: ts.TextRange;
    openBraceRange: ts.TextRange | undefined;
    isBlock: boolean;
    shouldBeBlock: boolean;
}

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

    getRange(statement: ts.Node): ts.TextRange {
        return {
            pos: statement.getStart(),
            end: statement.getEnd(),
        };
    }

    isSingleLine(statement: ts.Node): boolean {
        const start = this.sourceFile.getLineAndCharacterOfPosition(statement.getStart());
        const end = this.sourceFile.getLineAndCharacterOfPosition(statement.getEnd());
        return start.line === end.line;
    }

    analyzeNode(node: ts.Node): NodeAnalyzeResult;
    analyzeNode(node: ts.Node | undefined): UndefinedAnalyzeResult | NodeAnalyzeResult;
    analyzeNode(node: ts.Node | undefined): UndefinedAnalyzeResult | NodeAnalyzeResult {
        if (node === undefined)
            return { type: NodeType.Undefined };

        if (ts.isBlock(node)) {
            const result = new BlockAnalyzeResult();
            this.visitBlock(node, result);
            return result;
        }

        const result = new BlockAnalyzeResult();
        if (this.visitSpecialNode(node, result))
            return new StatementAnalyzeResult(!result.ok);

        if (this.hasLeadingComments(node))
            return new StatementAnalyzeResult(false);

        const isSingleLine = this.isSingleLine(node);
        return new StatementAnalyzeResult(isSingleLine);
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

    collectBranches(node: ts.IfStatement): Branch[] {
        const result: Branch[] = [];

        const then = node.thenStatement;
        if (ts.isBlock(then)) {
            result.push({
                keyword: "if",
                keywordRange: this.getRange(node.getChildAt(0)),
                openBraceRange: this.getRange(then.getChildAt(0)),
                isBlock: true,
                shouldBeBlock: this.analyzeNode(then).shouldBeBlock
            });
        } else {
            result.push({
                keyword: "if",
                keywordRange: this.getRange(node.getChildAt(0)),
                openBraceRange: undefined,
                isBlock: false,
                shouldBeBlock: this.analyzeNode(then).shouldBeBlock
            });
        }

        const _else = node.elseStatement;
        if (_else === undefined)
            return result;

        if (ts.isIfStatement(_else)) {
            for (const item of this.collectBranches(_else)) {
                if (item.keyword === "if") {
                    result.push({
                        keyword: "else if",
                        keywordRange: {
                            pos: node.getChildAt(5).pos,
                            end: item.keywordRange.end,
                        },
                        openBraceRange: item.openBraceRange,
                        isBlock: item.isBlock,
                        shouldBeBlock: item.shouldBeBlock
                    });
                }
                else {
                    result.push(item);
                }
            }
            return result;
        }

        if (ts.isBlock(_else)) {
            result.push({
                keyword: "else",
                keywordRange: this.getRange(node.getChildAt(5)),
                openBraceRange: this.getRange(_else.getChildAt(0)),
                isBlock: true,
                shouldBeBlock: this.analyzeNode(_else).shouldBeBlock,
            });
            return result;
        }

        result.push({
            keyword: "else",
            keywordRange: this.getRange(node.getChildAt(5)),
            openBraceRange: undefined,
            isBlock: false,
            shouldBeBlock: this.analyzeNode(_else).shouldBeBlock,
        });
        return result;
    }

    visitIfStatement(node: ts.IfStatement, result?: BlockAnalyzeResult): void {
        const SyntaxKind = ts.SyntaxKind;

        const branches = this.collectBranches(node);
        let shouldBeBlock = false;
        for (const item of branches) {
            if (item.shouldBeBlock) {
                shouldBeBlock = true;
                break;
            }
        }

        if (shouldBeBlock) {
            for (const item of branches)
                if (!item.isBlock)
                    super.addFailure(item.keywordRange.pos, item.keywordRange.end, `This "${item.keyword}" statement needs braces.`);
        } else {
            for (const item of branches)
                if (item.isBlock)
                    super.addFailure(item.openBraceRange!.pos, item.openBraceRange!.end, `This "${item.keyword}" statement doesn't need braces.`);
        }

        if (result !== undefined && !result.ok)
            if (shouldBeBlock || branches.length != 1)
                result.ok = true;
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
        const statementState = this.analyzeNode(statement);

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
