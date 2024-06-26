#include "taichi/ir/ir.h"
#include "taichi/ir/frontend_ir.h"
#include "taichi/ir/statements.h"

namespace taichi::lang {

class FrontendTypeCheck : public IRVisitor {
  void check_cond_type(const Expr &cond, std::string stmt_name) {
    if (!cond->ret_type->is<PrimitiveType>() || !is_integral(cond->ret_type))
      throw TaichiTypeError(fmt::format(
          "`{0}` conditions must be an integer; found {1}. Consider using "
          "`{0} x != 0` instead of `{0} x` for float values.",
          stmt_name, cond->ret_type->to_string()));
  }

 public:
  explicit FrontendTypeCheck() {
    allow_undefined_visitor = true;
  }

  void visit(Block *block) override {
    std::vector<Stmt *> stmts;
    // Make a copy since type casts may be inserted for type promotion.
    for (auto &stmt : block->statements)
      stmts.push_back(stmt.get());
    for (auto stmt : stmts)
      stmt->accept(this);
  }

  void visit(FrontendExternalFuncStmt *stmt) override {
    // TODO: noop for now; add typechecking after we have type specification
  }

  void visit(FrontendExprStmt *stmt) override {
    // Noop
  }

  void visit(FrontendAllocaStmt *stmt) override {
    // Noop
  }

  void visit(FrontendSNodeOpStmt *stmt) override {
    // Noop
  }

  void visit(FrontendAssertStmt *stmt) override {
    check_cond_type(stmt->cond, "assert");
  }

  void visit(FrontendAssignStmt *stmt) override {
    auto lhs_type = stmt->lhs->ret_type;
    auto rhs_type = stmt->rhs->ret_type;

    auto error = [&]() {
      throw TaichiTypeError(fmt::format("{}cannot assign '{}' to '{}'",
                                        stmt->tb, rhs_type->to_string(),
                                        lhs_type->to_string()));
    };

    // No implicit cast at frontend for now
    if (is_tensor(lhs_type) && is_tensor(rhs_type) &&
        lhs_type.get_shape() != rhs_type.get_shape()) {
      error();
    }
  }

  void visit(FrontendIfStmt *stmt) override {
    // TODO: use PrimitiveType::u1 when it's supported
    check_cond_type(stmt->condition, "if");
    if (stmt->true_statements)
      stmt->true_statements->accept(this);
    if (stmt->false_statements)
      stmt->false_statements->accept(this);
  }

  void visit(FrontendPrintStmt *stmt) override {
    // Noop
  }

  void visit(FrontendForStmt *stmt) override {
    stmt->body->accept(this);
  }

  void visit(FrontendFuncDefStmt *stmt) override {
    stmt->body->accept(this);
    // Determine ret_type after this is actually used
  }

  void visit(FrontendBreakStmt *stmt) override {
    // Noop
  }

  void visit(FrontendContinueStmt *stmt) override {
    // Noop
  }

  void visit(FrontendWhileStmt *stmt) override {
    check_cond_type(stmt->cond, "while");
    stmt->body->accept(this);
  }

  void visit(FrontendReturnStmt *stmt) override {
    // Noop
  }
};

namespace irpass {

void frontend_type_check(IRNode *root) {
  TI_AUTO_PROF;
  FrontendTypeCheck checker;
  root->accept(&checker);
}

}  // namespace irpass

}  // namespace taichi::lang
