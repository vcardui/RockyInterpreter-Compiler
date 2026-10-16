/*
15 de Octubre de 2026
Paola Montserrat Osorio García - 216511
Pablo David Pérez López - 300452
Vanessa Reteguín - 375533

Compilador Rocky

Universidad Aútonoma de Aguascalientes
Ingeniería en Computación Inteligente (ICI)
Semestre VII

Grupo: A
Materia: Autómatas II
Profesor: Braulio Jesús Montoya Padilla

Instrucciones: Crear un compilador con operaciones básicas
para el lenguaje que creamos en clase [Rocky]:

1. Análisis léxico
2. Análisis sintáctico
3. Análisis semántico
4. Greneración de código intermedio
5. Optimización
6. Generación de codigo objetivo
*/

#include <iostream>
#include <string> // std::to_string(num)
#include <sstream>
#include <vector>
#include <map>

/* ------------ Variables ------------ */
// Sólo vamos a admitir int o str
enum DataType {
    TYPE_INT,
    TYPE_STR
};

struct SymbolInfo {
    std::string value;
    std::string asmName;
};

// Global Symbol Table
std::map<std::string, SymbolInfo> symbolTable;
char varCount = 65;

enum IrOpcode {
    IR_ASSIGN_INT,
    IR_ASSIGN_STR,
    IR_STAMP_RAW,
    IR_STAMP_VAR,
    IR_ADD_INT
};

// Estructura válida para código intermedio (IR)
struct IrInstruction {
    IrOpcode operation;
    std::string name;
    std::string value;
    std::string asmName;
};

std::vector<IrInstruction> irCodeStream;

/* ------------ Parser (código intermedio) ------------ */
void generateIR(const std::string& input) {
    if (input.empty()){
        return;
    }

    std::stringstream ss(input);
    std::string firstToken;
    ss >> firstToken;

    // stampvar()
    if (firstToken.rfind("stampvar", 0) == 0) {
        size_t startParam = input.find('(');
        size_t endParam = input.rfind(')');
        if (startParam == std::string::npos || endParam == std::string::npos) {
            return;
        }

        std::string content = input.substr(startParam + 1, endParam - startParam - 1);
        size_t firstQuote = content.find('"');
        size_t lastQuote = content.rfind('"');
        if (firstQuote == std::string::npos || lastQuote == std::string::npos) {
            return;
        }

        std::string formatStr = content.substr(firstQuote + 1, lastQuote - firstQuote - 1);

        for (size_t i = 0; i < formatStr.length(); ++i) {
            if (formatStr[i] == '{') {
                size_t closeBrace = formatStr.find('}', i);
                if (closeBrace == std::string::npos){
                    return;
                }
                std::string varName = formatStr.substr(i + 1, closeBrace - i - 1);

                std::string currentVal = "";
                std::string currentAsm = "";
                if (symbolTable.find(varName) != symbolTable.end()) {
                    currentVal = symbolTable[varName].value;
                    currentAsm = symbolTable[varName].asmName;
                }

                irCodeStream.push_back({IR_STAMP_VAR, varName, currentVal, currentAsm});
                i = closeBrace;
            } else {
                size_t nextBrace = formatStr.find('{', i);
                std::string rawChunk = formatStr.substr(i, nextBrace - i);
                irCodeStream.push_back({IR_STAMP_RAW, rawChunk, "", ""});
                if (nextBrace == std::string::npos){
                    i = formatStr.length();
                } else {
                    i = nextBrace - 1;
                }
            }
        }
        return;
    }

    // Declaración de variables
    std::string name, equalsSign;
    if (!(ss >> name >> equalsSign) || equalsSign != "=") {
        return;
    }

    std::string varASM;
    if (firstToken == "int") {
        std::string expression;
        std::getline(ss >> std::ws, expression);

        size_t plusPos = expression.find('+');
        if (plusPos != std::string::npos) {
            std::string leftVar = expression.substr(0, plusPos);
            std::string rightVar = expression.substr(plusPos + 1);

            // Trim whitespace
            leftVar.erase(0, leftVar.find_first_not_of(" \t\r\n"));
            leftVar.erase(leftVar.find_last_not_of(" \t\r\n") + 1);
            rightVar.erase(0, rightVar.find_first_not_of(" \t\r\n"));
            rightVar.erase(rightVar.find_last_not_of(" \t\r\n") + 1);

            varASM = std::string(1, varCount) + "X";

            std::string leftReg = symbolTable.count(leftVar) ? symbolTable[leftVar].asmName : "0";
            std::string rightReg = symbolTable.count(rightVar) ? symbolTable[rightVar].asmName : "0";

            irCodeStream.push_back({IR_ADD_INT, name, leftReg, varASM});
            irCodeStream.back().value = leftReg;
            irCodeStream.back().name = rightReg;

            symbolTable[name] = {"0", varASM};
            varCount++;
        }
        else {
            std::stringstream exprStream(expression);
            std::string intValStr;
            if (exprStream >> intValStr) {
                varASM = std::string(1, varCount) + "X";
                irCodeStream.push_back({IR_ASSIGN_INT, name, intValStr, varASM});
                symbolTable[name] = {intValStr, varASM};
                varCount++;
            }
        }
    }
    else if (firstToken == "str") {
        std::string rawStr;
        std::getline(ss >> std::ws, rawStr);
        if (rawStr.front() == '"' && rawStr.back() == '"') {
            rawStr = rawStr.substr(1, rawStr.length() - 2);
        }
        varASM = std::string(1, varCount) + "X";
        irCodeStream.push_back({IR_ASSIGN_STR, name, rawStr, varASM});

        symbolTable[name] = {rawStr, varASM};
        varCount++;
    }
}

/* ------------ Imprimir código intermedio ------------ */
void printIntermediateCode() {
    for (const auto& instruction : irCodeStream) {
        switch (instruction.operation) {
            case IR_ASSIGN_INT:
                std::cout << "STORE_INT " << instruction.name << ", " << instruction.value << "\n";
                break;
            case IR_ASSIGN_STR:
                std::cout << "STORE_STR " << instruction.name << ", \"" << instruction.value << "\"\n";
                break;
            case IR_STAMP_RAW:
                std::cout << "PRINT_RAW \"" << instruction.name << "\"\n";
                break;
            case IR_STAMP_VAR:
                std::cout << "PRINT_VAR " << instruction.name << "\n";
                break;
            case IR_ADD_INT:
                std::cout << "ADD_INT " << instruction.asmName << " = " << instruction.value << " + " << instruction.name << "\n";
                break;
        }
    }
}

/* ------------ Imprimir código objetivo ------------ */
void printObjectiveCode() {
    for (const auto& instruction : irCodeStream) {
        std::cout << "0x" << std::hex << (int)instruction.operation << " ";
        for (char c : instruction.name) {
            std::cout << "0x" << std::hex << (int)c << " ";
        }
        std::cout << "0x00 "; // Separador
    }
    std::cout << std::dec << "\n";
}

/* ------------ Imprimir "ejecutable" (código ensamblador) ------------ */
void printAssembly() {
    std::cout << "    start:\n";

    for (const auto& instruction : irCodeStream) {
        if (instruction.operation == IR_ASSIGN_INT) {
            std::cout << "\n    # " << instruction.name << " = " << instruction.value << "\n";
            std::cout << "    mov " << instruction.asmName << ", " << instruction.value << "\n";
        }
        else if (instruction.operation == IR_STAMP_RAW) {
            std::cout << "\n    # Print string " << instruction.name << std::endl;
            std::cout << "    call print_string \"" << instruction.name << "\"" << std::endl;
        }
        else if (instruction.operation == IR_STAMP_VAR) {
            std::cout << "\n    # Print variable {" << instruction.name << "} with value \"" << instruction.value << "\"\n";
            std::cout << "    call print_int " << instruction.asmName << std::endl;
        }
        else if (instruction.operation == IR_ADD_INT) {
            std::cout << "\n    # " << instruction.asmName << " = " << instruction.value << " + " << instruction.name << "\n";
            std::cout << "    mov " << instruction.asmName << ", " << instruction.value << "\n";
            std::cout << "    add " << instruction.asmName << ", " << instruction.name << "\n";
        }
    }
}

int main() {
    std::string line;
    std::string code;

    while (std::getline(std::cin, line)) {
        code += line + "\n";
        generateIR(line);
    }

    std::cout << "\n---<{ Código original (Rocky) }>---\n";
    std::cout << code << "\n";

    std::cout << "\n---<{ Código intermedio }>---\n";
    printIntermediateCode();

    std::cout << "\n---<{ Código Objetivo }>---\n   (Simulación de bytecode)\n";
    printObjectiveCode();

    std::cout << "\n---<{ \"Ejectutable (Ensamblador Intel x86_64)\" }>---\n";
    printAssembly();

    return 0;
}
