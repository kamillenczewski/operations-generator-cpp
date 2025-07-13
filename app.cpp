#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <random>
#include <unordered_map>
#include <memory>
#include <chrono>
#include <cmath>
#include <fstream>

using namespace std;

string join(const vector<string>& items, const string& separator)
{
    ostringstream stream;
    
    for (string item : items)
    {
        stream << item << separator;
    }

    string result = stream.str();
    result.erase(result.size() - separator.size());

    return result;
}



template<typename T>
T choice(const vector<T>& items, const vector<double>& weights) {
    static mt19937 randomGenerator(random_device{}());

    discrete_distribution<int> distribution(weights.begin(), weights.end());

    auto index = distribution(randomGenerator);

    return items[index];
}

template<typename T>
vector<T> choices(const vector<T>& items, const vector<double>& weights, int number) 
{
    static mt19937 randomGenerator(random_device{}());

    discrete_distribution<int> distribution(weights.begin(), weights.end());

    vector<T> generatedItems;
    generatedItems.reserve(number);

    for (int i = 0; i < number; i++)
    {
        generatedItems.push_back(items[distribution(randomGenerator)]);
    }
    
    return generatedItems;
}

template<class T, class K>
vector<K> convert(vector<T> source, function<K(T)> conversionMethod)
{
    auto output = vector<K>(source.size());

    transform(source.begin(), source.end(), output.begin(), [conversionMethod](T obj) -> K {
        return conversionMethod(obj);
    });

    return output;
}



template <int tupleTypeIndex, class Tuple>
auto extractElements(const vector<Tuple>& items) 
{
    using ElementType = tuple_element_t<tupleTypeIndex, Tuple>;

    vector<ElementType> result;
    result.reserve(items.size());

    for (const auto& item : items) {
        result.push_back(get<tupleTypeIndex>(item));
    }

    return result;
}

template <class Tuple, size_t... tupleTypeIndexes>
auto preUnzip(const vector<Tuple>& items, index_sequence<tupleTypeIndexes...>) 
{
    return make_tuple(extractElements<tupleTypeIndexes>(items)...);
}

template <class... TupleTypes>
auto unzip(const vector<tuple<TupleTypes...>>& items) 
{
    return preUnzip(items, index_sequence_for<TupleTypes...>{});
}


auto countTime(function<void()> func)
{
    static auto multiplier = pow(10, -9);

    auto start = chrono::high_resolution_clock::now();
    func();
    auto end = chrono::high_resolution_clock::now();

    return chrono::duration_cast<chrono::nanoseconds>(end - start).count() * multiplier;
}


class Symbol
{
public:
    virtual string toString() = 0;
    virtual ~Symbol() = default;
};


class OperationType
{
public:
    virtual string toString(vector<shared_ptr<Symbol>> args) = 0;
    virtual int getArgsNumber() = 0;
    virtual ~OperationType() = default;
};


class SignOperationType: public OperationType
{
private:
    const string sign;
    const int argsNumber;
public:
    SignOperationType(string sign, int argsNumber):
        sign(sign), argsNumber(argsNumber)
    {
    }

    string toString(vector<shared_ptr<Symbol>> args) override
    {
        auto strings  = convert<shared_ptr<Symbol>, string>(args, [](shared_ptr<Symbol> symbol){
            return symbol->toString();
        });

        return join(strings, this->sign);
    }

    int getArgsNumber() override
    {
        return this->argsNumber;
    }
};

class NameOperationType: public OperationType
{
private:
    const string separator = ", ";
    const string name;
    const int argsNumber;
public:
    NameOperationType(string name, int argsNumber):
        name(name), argsNumber(argsNumber)
    {
    }

    string toString(vector<shared_ptr<Symbol>> args) override
    {
        auto strings = vector<string>(args.size());

        transform(args.begin(), args.end(), strings.begin(), [](shared_ptr<Symbol> symbol){
            return symbol->toString();
        });

        return name + "(" + join(strings, this->separator) + ")";
    }

    int getArgsNumber() override
    {
        return this->argsNumber;
    }
};


class Operation : public Symbol
{
private:
    shared_ptr<OperationType> type;
    vector<shared_ptr<Symbol>> args;
public:
    Operation(shared_ptr<OperationType> type, vector<shared_ptr<Symbol>> args):
        type(type), args(args)
    {
    }

    string toString() override
    {
        return this->type->toString(this->args);
    }
};


class Variable: public Symbol
{
private:
    const string name;
public:
    Variable(string name):
        name(name)
    {
    }

    string toString() override
    {
        return this->name;
    }
};

class Number: public Symbol
{
private:
    const int value;
public:
    Number(int value):
        value(value)
    {
    }

    string toString() override
    {
        return to_string(this->value);
    }
};



class OperationGenerator
{
private:
    vector<shared_ptr<Variable>> variables;
    vector<shared_ptr<OperationType>> operationTypes;
    vector<shared_ptr<Number>> numbers;

    vector<double> variablesWeights;
    vector<double> operationTypesWeights;
    vector<double> numbersWeights;

    unordered_map<string, double> objectsAndWeights;

    const vector<double> zeroDepthWeights = { this->objectsAndWeights["variable"], this->objectsAndWeights["number"] };
    const vector<double> anyDepthWeights = { this->objectsAndWeights["variable"], this->objectsAndWeights["number"], this->objectsAndWeights["operation"] };

    mt19937 randomGenerator = mt19937(random_device{}());
    discrete_distribution<int> zeroDepthDistribution = discrete_distribution<int>(this->zeroDepthWeights.begin(), zeroDepthWeights.end());
    discrete_distribution<int> anyDepthDistribution = discrete_distribution<int>(anyDepthWeights.begin(), anyDepthWeights.end());
public:
    OperationGenerator(
        vector<tuple<shared_ptr<Variable>, double>> variablesAndWeights, 
        vector<tuple<shared_ptr<OperationType>, double>> operationTypesAndWeights, 
        vector<tuple<shared_ptr<Number>, double>> numbersAndWeights, 
        unordered_map<string, double> objectsAndWeights
    ):
        objectsAndWeights(objectsAndWeights)
    {
        tie(this->variables, this->variablesWeights) = unzip(variablesAndWeights);
        tie(this->operationTypes, this->operationTypesWeights) = unzip(operationTypesAndWeights);
        tie(this->numbers, this->numbersWeights) = unzip(numbersAndWeights);
    }

    shared_ptr<OperationType> generateOperationType()
    {
        return choice(this->operationTypes, this->operationTypesWeights);
    }

    shared_ptr<Variable> generateVariable()
    {
        return choice(this->variables, this->variablesWeights);
    }

    shared_ptr<Number> generateNumber()
    {
        return choice(this->numbers, this->numbersWeights);
    }


    int generateZeroDepthObjectIndex()
    {
        return this->zeroDepthDistribution(this->randomGenerator);
    }

    int generateAnyDepthObjectIndex()
    {
        return this->anyDepthDistribution(this->randomGenerator);
    }

    shared_ptr<Symbol> generateSymbol(int depth)
    {
        int objectIndex;

        if (depth == 0)
        {
            objectIndex = this->generateZeroDepthObjectIndex();
            
            if (objectIndex == 0)
            {
                return this->generateVariable();
            }
            else
            {
                return this->generateNumber();
            }
        }

        objectIndex = this->generateAnyDepthObjectIndex();

        if (objectIndex == 0)
        {
            return this->generateVariable();
        }
        else if (objectIndex == 1)
        {
            return this->generateNumber();
        }
        else
        {
            return this->generateOperation(depth);
        }
    }

    vector<shared_ptr<Symbol>> generateSymbols(int depth, int amount)
    {
        vector<shared_ptr<Symbol>> symbols;
        symbols.reserve(amount);

        for (int i = 0; i < amount; i++)
        {
            symbols.push_back(this->generateSymbol(depth));
        }
        
        return symbols;
    }

    shared_ptr<Operation> generateOperation(int depth)
    {
        shared_ptr<OperationType> type = this->generateOperationType();
        return make_shared<Operation>(type, this->generateSymbols(depth - 1, type->getArgsNumber()));
    }

    vector<shared_ptr<Operation>> generateOperations(int depth, int amount)
    {
        vector<shared_ptr<Operation>> operations;
        operations.reserve(amount);

        for (int i = 0; i < amount; i++)
        {
            operations.push_back(this->generateOperation(depth));
        }
        
        return operations;
    }
};

string generateUUID(int length)
{
    static const char* possibleChars = "0123456789abcdefghijklmnopqrstuvwxyz";
    static const int charsNumber = 36;
    static mt19937 randomGenerator(random_device{}());
    static uniform_int_distribution<> distribution(0, charsNumber - 1);

    stringstream stream;

    for (int i = 0; i < length; i++)
    {
        stream << possibleChars[distribution(randomGenerator)];
    }
    
    return stream.str();
}

string generateOutputFileName()
{
    return "output_" + generateUUID(10) + ".txt";
}

void run1()
{
    vector<string> strings = {"Jablko", "Banan", "Pomarancz"};
    vector<double> weights = {1,2,3};

    auto items = choices<string>(strings, weights, 10);


    for (auto item : items)
    {
        cout << item << '\n';
    }    
}


void run2()
{
    vector<tuple<int, string, double>> tuples = 
    {
        {3, "Jablko", 0.12}, 
        {17, "banan", 3.14}
    };

    auto [a, b, c] = unzip(tuples);

    for (auto _: a)
    {
        cout << _ << '\n';
    }
    for (auto _: b)
    {
        cout << _ << '\n';
    }
    for (auto _: c)
    {
        cout << _ << '\n';
    }
}

void run3()
{
    const auto ADDITION = make_shared<SignOperationType>("+", 2);
    const auto MULTIPLICATION = make_shared<SignOperationType>("*", 2);
    const auto SUBTRACTION = make_shared<SignOperationType>("-", 2);
    const auto DIVISION = make_shared<SignOperationType>("/", 2);
    const auto EXPONENTIATION = make_shared<SignOperationType>("^", 2);

    const auto SIN = make_shared<NameOperationType>("sin", 1);
    const auto COS = make_shared<NameOperationType>("cos", 1);
    const auto LN = make_shared<NameOperationType>("ln", 1);
    const auto FLOOR = make_shared<NameOperationType>("floor", 1);

    vector<tuple<shared_ptr<OperationType>, double>> operationTypesAndWeights =
    {
        { ADDITION, 1 },
        { MULTIPLICATION, 1 },
        { SUBTRACTION, 1 },
        { DIVISION, 1 },
        { EXPONENTIATION, 1 },

        { SIN, 1 },
        { COS, 1 },
        { LN, 1 },
        { FLOOR, 1 }
    };

    vector<tuple<shared_ptr<Variable>, double>> variablesAndWeights =
    {
        { make_shared<Variable>("x"), 1}
    };

    vector<tuple<shared_ptr<Number>, double>> numbersAndWeights =
    {
        { make_shared<Number>(3), 1}
    };

    unordered_map<string, double> objectsAndWeights =
    {
        { "variable", 1},
        { "operation", 3},
        { "number", 1}
    };

    auto generator = OperationGenerator(variablesAndWeights, operationTypesAndWeights, numbersAndWeights, objectsAndWeights);

    int depth = 10;
    int amount = 10000;

    vector<shared_ptr<Operation>> operations;

    auto time = countTime([&generator, &operations, &depth, &amount](){
        operations = generator.generateOperations(depth, amount);
    });

    cout << "Time: " << fixed << time << '\n';

    ofstream file(generateOutputFileName());

    if (file.is_open()) 
    {
        for (auto& operation : operations)
        {
            file << operation->toString() + '\n';
        }

        file.close();
    }
}

int main()
{
    run3();

    return 0;
}