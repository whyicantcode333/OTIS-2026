#include <iostream>
#include <sstream>
#include <cassert>
#include <string>

#include "../src/ApplicationLogic.h"

using namespace std;

string runApplication(const string& input)
{
    istringstream inputStream(input);
    ostringstream outputStream;

    streambuf* oldCin = cin.rdbuf();
    streambuf* oldCout = cout.rdbuf();

    cin.rdbuf(inputStream.rdbuf());
    cout.rdbuf(outputStream.rdbuf());

    ApplicationLogic app;
    app.run();

    cin.rdbuf(oldCin);
    cout.rdbuf(oldCout);

    cin.clear();
    cout.clear();

    return outputStream.str();
}

void testSuccessfulSimulation()
{
    string output = runApplication("1\n1\n3\n");

    assert(output.find("Simulation results:") != string::npos);
    assert(output.find("Time") != string::npos);
    assert(output.find("Input") != string::npos);
    assert(output.find("Output") != string::npos);

    cout << "Successful simulation: PASSED" << endl;
}

void testInvalidModel()
{
    string output = runApplication("5\n");

    assert(output.find("Error: invalid model choice!") != string::npos);

    cout << "Invalid model choice: PASSED" << endl;
}

void testInvalidInput()
{
    string output = runApplication("1\n5\n");

    assert(output.find("Error: invalid input signal!") != string::npos);

    cout << "Invalid input signal: PASSED" << endl;
}

void testInvalidSteps()
{
    string output = runApplication("1\n1\n0\n");

    assert(output.find("Error: number of steps must be positive!") != string::npos);

    cout << "Invalid number of steps: PASSED" << endl;
}

int main()
{
    cout << "=== APPLICATION LOGIC TESTS ===" << endl;

    testSuccessfulSimulation();
    testInvalidModel();
    testInvalidInput();
    testInvalidSteps();

    cout << "All application logic tests passed!" << endl;

    return 0;
}