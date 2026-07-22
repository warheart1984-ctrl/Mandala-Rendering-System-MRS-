/**
 * Minimal CQL parser — SELECT / FROM / WHERE / ORDER BY / LIMIT.
 */

export function parseCql(query) {
  const q = query.trim().replace(/\s+/g, " ");
  const selectMatch = q.match(/^SELECT\s+(\w+)\s+FROM\s+(\w+)/i);
  if (!selectMatch) throw new Error("CQL must start with SELECT <field> FROM <source>");

  const ast = {
    select: selectMatch[1].toLowerCase(),
    from: selectMatch[2].toLowerCase(),
    where: null,
    orderBy: null,
    limit: null,
  };

  let rest = q.slice(selectMatch[0].length).trim();

  const whereIdx = rest.search(/\bWHERE\b/i);
  const orderIdx = rest.search(/\bORDER\s+BY\b/i);
  const limitIdx = rest.search(/\bLIMIT\b/i);

  const cut = (str, idx) => (idx >= 0 ? str.slice(0, idx).trim() : str);

  if (whereIdx >= 0) {
    const whereEnd = Math.min(
      orderIdx >= 0 ? orderIdx : rest.length,
      limitIdx >= 0 ? limitIdx : rest.length,
    );
    const whereStr = rest.slice(whereIdx + 5, whereEnd).trim();
    ast.where = parseWhere(whereStr);
    rest = rest.slice(whereEnd);
  }

  const orderMatch = rest.match(/ORDER\s+BY\s+([\w.]+)\s+(ASC|DESC)?/i);
  if (orderMatch) {
    ast.orderBy = {
      field: orderMatch[1],
      direction: (orderMatch[2] ?? "ASC").toUpperCase(),
    };
  }

  const limitMatch = rest.match(/LIMIT\s+(\d+)/i);
  if (limitMatch) ast.limit = Number(limitMatch[1]);

  return ast;
}

function parseWhere(whereStr) {
  if (!whereStr) return null;
  const andParts = splitLogical(whereStr, "AND");
  if (andParts.length > 1) {
    return andParts
      .slice(1)
      .reduce(
        (acc, part) => ({
          type: "logical",
          op: "AND",
          left: acc,
          right: parseWhere(part.trim()),
        }),
        parseWhere(andParts[0].trim()),
      );
  }
  const orParts = splitLogical(whereStr, "OR");
  if (orParts.length > 1) {
    return orParts
      .slice(1)
      .reduce(
        (acc, part) => ({
          type: "logical",
          op: "OR",
          left: acc,
          right: parseWhere(part.trim()),
        }),
        parseWhere(orParts[0].trim()),
      );
  }
  return parseBinary(whereStr.trim());
}

function splitLogical(str, op) {
  const re = new RegExp(`\\s+${op}\\s+`, "i");
  return str.split(re);
}

function parseBinary(expr) {
  const m = expr.match(/^([\w.]+)\s*(=|!=|>=|<=|>|<|CONTAINS)\s*(.+)$/i);
  if (!m) throw new Error(`Invalid predicate: ${expr}`);
  let value = m[3].trim();
  if (
    (value.startsWith('"') && value.endsWith('"')) ||
    (value.startsWith("'") && value.endsWith("'"))
  ) {
    value = value.slice(1, -1);
  }
  return { type: "binary", field: m[1], op: m[2].toUpperCase(), value };
}
