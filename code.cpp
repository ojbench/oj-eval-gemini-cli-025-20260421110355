#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <random>
#include <ctime>

#include "lang.h"
#include "transform.h"
#include "visitor.h"

// --- Cheat Implementation ---

class Renamer : public Transform {
    std::map<std::string, std::string> varMap;
    std::map<std::string, std::string> funcMap;
    int varCount = 0;
    int funcCount = 0;

    std::string getNewVar(const std::string& name) {
        if (varMap.find(name) == varMap.end()) {
            varMap[name] = "v" + std::to_string(++varCount);
        }
        return varMap[name];
    }

    std::string getNewFunc(const std::string& name) {
        if (name == "main" || builtinFunctions.count(name)) return name;
        if (funcMap.find(name) == funcMap.end()) {
            funcMap[name] = "f" + std::to_string(++funcCount);
        }
        return funcMap[name];
    }

public:
    Program* transformProgram(Program* node) override {
        for (auto decl : node->body) {
            getNewFunc(decl->name);
        }
        
        std::vector<FunctionDeclaration*> body;
        for (auto decl : node->body) {
            body.push_back(transformFunctionDeclaration(decl));
        }
        std::shuffle(body.begin(), body.end(), std::mt19937(std::time(0)));
        return new Program(body);
    }

    FunctionDeclaration* transformFunctionDeclaration(FunctionDeclaration* node) override {
        varMap.clear();
        std::vector<Variable*> params;
        for (auto param : node->params) {
            params.push_back(new Variable(getNewVar(param->name)));
        }
        return new FunctionDeclaration(getNewFunc(node->name), params, transformStatement(node->body));
    }

    Variable* transformVariable(Variable* node) override {
        return new Variable(getNewVar(node->name));
    }

    Expression* transformCallExpression(CallExpression* node) override {
        std::vector<Expression*> args;
        for (auto arg : node->args) {
            args.push_back(transformExpression(arg));
        }
        return new CallExpression(getNewFunc(node->func), args);
    }

    Expression* transformExpression(Expression* node) override {
        Expression* e = Transform::transformExpression(node);
        if (rand() % 20 == 0) {
            return new CallExpression("+", {e, new IntegerLiteral(0)});
        }
        return e;
    }

    Statement* transformBlockStatement(BlockStatement* node) override {
        std::vector<Statement*> body;
        for (auto stmt : node->body) {
            body.push_back(transformStatement(stmt));
            if (rand() % 20 == 0) {
                body.push_back(new SetStatement(new Variable("dummy" + std::to_string(rand() % 100)), new IntegerLiteral(0)));
            }
        }
        return new BlockStatement(body);
    }
};

// --- Anticheat Implementation ---

class Canonicalizer : public Visitor<std::string> {
public:
    std::string visitProgram(Program* node) override {
        std::vector<std::string> funcs;
        for (auto func : node->body) {
            funcs.push_back(visitFunctionDeclaration(func));
        }
        std::sort(funcs.begin(), funcs.end());
        std::string res;
        for (const auto& s : funcs) res += s + "|";
        return res;
    }

    std::string visitFunctionDeclaration(FunctionDeclaration* node) override {
        return "func(" + std::to_string(node->params.size()) + ")" + visitStatement(node->body);
    }

    std::string visitExpressionStatement(ExpressionStatement* node) override {
        return "expr(" + visitExpression(node->expr) + ")";
    }

    std::string visitSetStatement(SetStatement* node) override {
        return "set(" + visitExpression(node->value) + ")";
    }

    std::string visitIfStatement(IfStatement* node) override {
        return "if(" + visitExpression(node->condition) + "," + visitStatement(node->body) + ")";
    }

    std::string visitForStatement(ForStatement* node) override {
        return "for(" + visitStatement(node->init) + "," + visitExpression(node->test) + "," + 
               visitStatement(node->update) + "," + visitStatement(node->body) + ")";
    }

    std::string visitBlockStatement(BlockStatement* node) override {
        std::string res = "block{";
        for (auto stmt : node->body) {
            res += visitStatement(stmt) + ";";
        }
        res += "}";
        return res;
    }

    std::string visitReturnStatement(ReturnStatement* node) override {
        return "ret(" + visitExpression(node->value) + ")";
    }

    std::string visitIntegerLiteral(IntegerLiteral* node) override {
        return "int";
    }

    std::string visitVariable(Variable* node) override {
        return "var";
    }

    std::string visitCallExpression(CallExpression* node) override {
        std::string res = "call:" + (builtinFunctions.count(node->func) ? node->func : "user") + "(";
        for (auto arg : node->args) {
            res += visitExpression(arg) + ",";
        }
        res += ")";
        return res;
    }
};

int main() {
    srand(time(0));
    Program* prog1 = scanProgram(std::cin);
    
    removeWhitespaces(std::cin);
    if (std::cin.eof()) {
        Renamer renamer;
        Program* cheated = renamer.transformProgram(prog1);
        std::cout << cheated->toString() << std::endl;
    } else {
        Program* prog2 = scanProgram(std::cin);
        
        Canonicalizer canon;
        std::string s1 = canon.visitProgram(prog1);
        std::string s2 = canon.visitProgram(prog2);
        
        if (s1 == s2) {
            std::cout << "1.0" << std::endl;
        } else {
            int l1 = s1.length();
            int l2 = s2.length();
            if (l1 == 0 || l2 == 0) {
                std::cout << "0.0" << std::endl;
                return 0;
            }
            double sim = 1.0 - (double)std::abs(l1 - l2) / std::max(l1, l2);
            
            if (sim > 0.8) {
                std::cout << 0.5 + 0.5 * (sim - 0.8) / 0.2 << std::endl;
            } else {
                std::cout << 0.5 * sim / 0.8 << std::endl;
            }
        }
    }
    return 0;
}
