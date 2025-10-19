#include <vector>
#include <map>
#include <optional>
#include <stack>
#include "Absyn.H"



struct FnType {
    Type *ret;
    std::vector<const Type*> args; 
};

struct StType {
    //std::optional<Id> parent;
    bool is_derived = false; // true if this is a derived struct
    bool isException = false; // true if this is an exception struct
    std::map<Id, const Type*> members;
};

struct VaType { const Type* type; };

template<typename T> using SymbolTable = std::map<Id, T>;

using FnTable = SymbolTable<FnType>;
using StTable = SymbolTable<StType>;
using VaTable = SymbolTable<VaType>;

struct Globals { FnTable fns; StTable sts; };
using Scope = VaTable;

struct Context {
    Globals globals;
    std::vector<Scope> vars;
};
