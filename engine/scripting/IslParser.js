/**
 * ISL v2.0 lexer + parser (authoritative JS implementation).
 *
 * intent <verb>(<args?>) in world("<id>") [at "<tc>"] [with evidence("<id>") | params { ... }] ;
 */

import { IntentNode, ProgramNode } from "./IslAst.js";

class Lexer {
  constructor(source) {
    this.src = source;
    this.i = 0;
  }

  tokenize() {
    const tokens = [];
    while (this.i < this.src.length) {
      const c = this.src[this.i];
      if (/\s/.test(c)) {
        this.i++;
        continue;
      }
      if (c === "/" && this.src[this.i + 1] === "/") {
        while (this.i < this.src.length && this.src[this.i] !== "\n") this.i++;
        continue;
      }
      if (c === '"' || c === "'") {
        tokens.push(this.readString(c));
        continue;
      }
      if (this.isNumberStart()) {
        tokens.push(this.readNumber());
        continue;
      }
      if (/[A-Za-z_]/.test(c)) {
        tokens.push(this.readIdent());
        continue;
      }
      if ("(),{};:[]".includes(c)) {
        tokens.push({ type: c, value: c });
        this.i++;
        continue;
      }
      throw new Error(`ISL lex error at ${this.i}: unexpected '${c}'`);
    }
    tokens.push({ type: "eof", value: null });
    return tokens;
  }

  isNumberStart() {
    return /^-?\d/.test(this.src.slice(this.i));
  }

  readString(q) {
    this.i++;
    let s = "";
    while (this.i < this.src.length && this.src[this.i] !== q) {
      if (this.src[this.i] === "\\") {
        this.i++;
        s += this.src[this.i++] ?? "";
      } else {
        s += this.src[this.i++];
      }
    }
    if (this.src[this.i] !== q) throw new Error("Unterminated string in ISL");
    this.i++;
    return { type: "string", value: s };
  }

  readNumber() {
    const start = this.i;
    if (this.src[this.i] === "-") this.i++;
    while (this.i < this.src.length && /[0-9.]/.test(this.src[this.i])) this.i++;
    return { type: "number", value: Number(this.src.slice(start, this.i)) };
  }

  readIdent() {
    const start = this.i;
    while (this.i < this.src.length && /[A-Za-z0-9_]/.test(this.src[this.i])) this.i++;
    const value = this.src.slice(start, this.i);
    const keywords = new Set([
      "intent",
      "world",
      "timeline",
      "entity",
      "asset",
      "at",
      "with",
      "evidence",
      "params",
      "in",
    ]);
    return { type: keywords.has(value) ? value : "ident", value };
  }
}

class Parser {
  constructor(tokens) {
    this.tokens = tokens;
    this.i = 0;
  }

  peek() {
    return this.tokens[this.i];
  }

  eat(type) {
    const t = this.peek();
    if (t.type !== type) {
      throw new Error(`ISL parse error: expected ${type}, got ${t.type} (${JSON.stringify(t.value)})`);
    }
    this.i++;
    return t;
  }

  parse() {
    const stmts = [];
    while (this.peek().type !== "eof") {
      stmts.push(this.parseIntent());
      if (this.peek().type === ";") this.eat(";");
    }
    return ProgramNode(stmts);
  }

  parseIntent() {
    this.eat("intent");
    const verb = this.eat("ident").value;
    this.eat("(");
    const args = [];
    if (this.peek().type !== ")") {
      args.push(this.parseValue());
      while (this.peek().type === ",") {
        this.eat(",");
        args.push(this.parseValue());
      }
    }
    this.eat(")");
    this.eat("in");
    this.eat("world");
    this.eat("(");
    const worldId = this.eat("string").value;
    this.eat(")");

    let at = null;
    if (this.peek().type === "at") {
      this.eat("at");
      at = this.eat("string").value;
    }

    let evidence = null;
    if (this.peek().type === "with") {
      this.eat("with");
      if (this.peek().type === "evidence") {
        this.eat("evidence");
        this.eat("(");
        evidence = { kind: "evidence", id: this.eat("string").value };
        this.eat(")");
      } else if (this.peek().type === "params") {
        this.eat("params");
        evidence = { kind: "params", object: this.parseObject() };
      } else {
        throw new Error("ISL: after 'with' expected evidence(...) or params {...}");
      }
    }

    return IntentNode(verb, args, worldId, at, evidence);
  }

  parseValue() {
    const t = this.peek();
    if (t.type === "string" || t.type === "number") {
      this.i++;
      return t.value;
    }
    if (t.type === "{") return this.parseObject();
    if (t.type === "[") return this.parseArray();
    throw new Error(`ISL: invalid value ${t.type}`);
  }

  parseObject() {
    this.eat("{");
    const obj = {};
    if (this.peek().type !== "}") {
      this.parseField(obj);
      while (this.peek().type === ",") {
        this.eat(",");
        if (this.peek().type === "}") break;
        this.parseField(obj);
      }
    }
    this.eat("}");
    return obj;
  }

  parseField(obj) {
    const key = this.eat("ident").value;
    this.eat(":");
    obj[key] = this.parseValue();
  }

  parseArray() {
    this.eat("[");
    const arr = [];
    if (this.peek().type !== "]") {
      arr.push(this.parseValue());
      while (this.peek().type === ",") {
        this.eat(",");
        if (this.peek().type === "]") break;
        arr.push(this.parseValue());
      }
    }
    this.eat("]");
    return arr;
  }
}

export function parseIsl(source) {
  const tokens = new Lexer(source).tokenize();
  return new Parser(tokens).parse();
}
