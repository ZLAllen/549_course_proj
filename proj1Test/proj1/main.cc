#include <iostream>
#include <fstream> 
#include <vector>
#include <time.h>
#include "parse_bench.tab.h"
#include "ClassCircuit.h"
#include "ClassGate.h"

using namespace std;

// Just for the parser
extern "C" int yyparse();
extern FILE *yyin; // Input file for parser

// Our circuit. We declare this external so the parser can use it more easily.
extern Circuit* myCircuit;

vector<char> constructInputLine(string line);


//----------------
// If you add functions, please add the prototypes here.
void findOutputVal(Gate* myGate);
int nand2(int a1, int a2);
int nor2(int a1, int a2);
int and2(int a1, int a2);
int or2(int a1, int a2);
int xor2(int a1, int a2);
int xnor2(int a1, int a2);
int buff2(int a1, int a2);
int not2(int a1, int a2);
int notd(int a1);


/*
 *
 * The following array initialization is only available in c11
 * please check if the flag -std=c++11 is presented in makefile
 *
 *
 *
 * 
 */
int (*p[8])(int, int){nand2,nor2,and2,or2,xor2,xnor2,buff2,not2};
int and_lookup[4][4]{
    {0,0,0,0},{0,1,2,3},{0,2,2,0},{0,3,0,3}
};
int or_lookup[4][4]{
    {0,1,2,3},{1,1,1,1},{2,1,2,1},{3,1,1,3}
};
int xor_lookup[4][4]{
    {0,1,2,3},{1,0,3,2},{2,3,0,1},{3,2,1,0}
};


//-----------------


int main(int argc, char* argv[]) {

  // Check the command line input and usage
  if (argc != 4) {
    cout << "Usage: ./logicsim circuit_file input_vectors output_location" << endl;
    return 1;
  }

  // Parse the bench file and initialize the circuit. (Using C style for our parser.)
  FILE *benchFile = fopen(argv[1], "r");
  if (benchFile == NULL) {
    cout << "ERROR: Cannot read file " << argv[1] << " for input" << endl;
    return 1;
  }
  yyin=benchFile;
  yyparse();
  fclose(benchFile);

  myCircuit->setupCircuit(); 
  cout << endl;
 
  // Setup the input vector file 
  vector<vector<char> > inputValues;
  ifstream inputStream; 
  inputStream.open(argv[2] );
  string inputLine;
  if (!inputStream.is_open()) {
    cout << "ERROR: Cannot read file " << argv[2] << " for input" <<    endl;
    return 1;
  }

  // Setup the output text file
  ofstream outputStream;
  outputStream.open(argv[3]);
  if (!outputStream.is_open()) {
    cout << "ERROR: Cannot open file " << argv[3] << " for output" << endl;
    return 1;
  }

  // Print the circuit structure. This is just so you can see how it works. Feel free to
  // comment this out. This function will also print any set logic values, so this can help
  // you in debugging.
  myCircuit->printAllGates();    

  
  // Try to read a line of inputs from the file.
  while(getline(inputStream, inputLine)) {

    
    // clear logic values of my circuit
    myCircuit->clearGateValues();

    // set new logic values
    myCircuit->setPIValues(constructInputLine(inputLine));

    // ---------------------------------------------
    // Write your code here!
    //
    // Compute the logic values of all gates in the circuit. When you
    // have calculated the logic value for a gate, use the setValue funtion
    // on it.

    // If you define other functions, place them at the bottom of this file 
    // and place the prototypes in the marked region above.
    //
    // Do *not* edit any of the other source files (ClassGate, ClassCircuit, etc.).

    // If you think there's something missing from the API that you need to solve
    // the problem, perhaps I missed something. Please let me know and we can discuss.
    
    vector<Gate*> circuitPOs = myCircuit->getPOGates();
    for(int i = 0; i < circuitPOs.size(); ++i){
        findOutputVal(circuitPOs[i]);
    }

    // Getting started:
    //   So now you have myCircuit, a pointer to a Circuit object, and you can
    //   get a vector<Gate*> of the circuit's input or output gates by running
    //   vector<Gate*> circuitPOs = myCircuit->getPOGates();
    //     and
    //   vector<Gate*> circuitPIs = myCircuit->getPIGates();

    // Once you have a Gate*, you can use its functions to go to its predecessors
    // and successors. 
    //
    // Here are two ideas for how you can set the circuit values in the gate:
    // 

    // Option 1: Do a breadth first traversal of the gates from the inputs.
    // Keep a queue of gates to examine. Start by putting all PI gates on
    // the queue. Then iteratively pop a Gate* from the queue, and check to
    // see if its input values are set. If they aren't put this Gate* back onto
    // the end of the queue. If they are set, calculate this gate's new
    // value (based on its gate type and the values of its inputs). Then
    // add this gate's successors (the gates that this gate's output goes to)
    // into the queue. Repeat until the queue is empty.
   
    // Option 2: You can write a recursive function starting from the POs.
    // If this gate's input values are unknown, recurse on the inputs to
    // assign their values. Eventually your recursive calls finish and
    // this gate's input values are known. Then set its output value
    // and return.

    // Feel free to delete these long comments when you don't need them anymore.

    
    // stop writing your code here.
    // ---------------------------------------------
    

    // Write the results we just simulated to the output file
    vector<Gate*> outputGates = myCircuit->getPOGates();
    for (int i=0; i < outputGates.size(); i++) {
      outputStream << outputGates[i]->printValue();
    }
    outputStream << endl;       
  }
  
  // close the input and output streams
  outputStream.close();
  inputStream.close();

  return 0;
}


// A function to help read in the input values from the text file. 
// You don't need to touch this.
vector<char> constructInputLine(string line) {
  
  vector<char> inputVals;
  
  for (int i=0; i<line.size(); i++) {
    if (line[i] == '0') 
      inputVals.push_back(LOGIC_ZERO);
    
    else if (line[i] == '1') 
      inputVals.push_back(LOGIC_ONE);

    else if ((line[i] == 'X') || (line[i] == 'x'))
      inputVals.push_back(LOGIC_X);
   
    else if ((line[i] == 'D') || (line[i] == 'd'))
      inputVals.push_back(LOGIC_D);

    else if ((line[i] == 'B') || (line[i] == 'b'))
      inputVals.push_back(LOGIC_DBAR);

    else {
      cout << "ERROR: Do not recognize character " << line[i] << " in line " << i+1 << " of input vector file. Setting to X" << endl;
      inputVals.push_back(LOGIC_X);
    }
  }
  
  return inputVals;

}

////////////////////////////////////////////////
// Please place any functions you add here, between these bars.

void findOutputVal(Gate* myGate){
    char type = myGate->get_gateType();
    if(type == GATE_PI){
           return;
    }else{
       vector<Gate*> inputGates = myGate->get_gateInputs();
       int result = 5;
       int xflag = 0;
       for(int i=0; i<inputGates.size(); i++){
           Gate* a = inputGates[i];
           if(a->getValue() == LOGIC_UNSET){
               findOutputVal(a);
           }
           if(a->getValue() == LOGIC_X){
               xflag = 1;
               continue;
           }
           result = (*p[type])((int)a->getValue(), result);
        }

       if(xflag){
           result = (*p[type])(4, result);
       }
       
       if(type == GATE_NAND || type == GATE_NOR || type == GATE_XNOR){
           result = not2(result, 5);
       }


       myGate->setValue(result);
    }
}

int nand2(int a1, int a2){
    return and2(a1,a2);
}

int nor2(int a1, int a2){
    return or2(a1,a2);
}

int and2(int a1, int a2){
    if(a2 == 5) return a1;
    else if(a1 == 0 || a2 == 0) return 0;
    else if(a1 == 4 || a2 == 4) return 4;
    else return and_lookup[a1][a2];
}

int or2(int a1, int a2){
    if(a2 == 5) return a1;
    else if(a1 == 1 || a2 == 1) return 1;
    else if(a1 == 4 || a2 == 4) return 4;
    else return or_lookup[a1][a2];
}

int xnor2(int a1, int a2){
    return xor2(a1,a2);
}

int buff2(int a1, int a2){
    return a1;
}

int xor2(int a1, int a2){
    if(a2 == 5) return a1;
    else if(a1 == 4 || a2 == 4) return 4;
    else return xor_lookup[a1][a2];
}

int not2(int a1, int a2){
    if(a1 == 4) return 4;
    else if(a1 == 0) return 1;
    else if (a1 == 1) return 0;
    else return notd(a1);
}

int notd(int a1){
    return a1 == 2? 3:2;
}
////////////////////////////////////////////////

