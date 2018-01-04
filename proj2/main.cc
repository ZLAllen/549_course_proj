// ESE-549 Project 2 
// YOUR NAME HERE

#include <iostream>
#include <fstream> 
#include <vector>
#include <time.h>
#include <algorithm>
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

bool faultSortFunc(Fault a, Fault b);
vector<Fault> sortFaultList(vector<Fault> v);
void printFaultList(vector<Fault> l);


//--------------------------------------------------
// Add the function prototypes for any new functions you add here, between these lines.

void findOutputVal(Gate* myGate);
void processControlFault(Gate* gate, int conVal);
void processSpecialFault(Gate* gate);
void processFlowFault(Gate* gate);
vector<Fault> faultUnion(vector<Fault>a, vector<Fault>b);
vector<Fault> faultIntersect(vector<Fault>a, vector<Fault>b);
vector<Fault> faultMinus(vector<Fault>a, vector<Fault>b);
vector<Fault> faultSymDiff(vector<Fault>a, vector<Fault>b);
int nand2(int a1, int a2);
int nor2(int a1, int a2);
int and2(int a1, int a2);
int or2(int a1, int a2);
int xor2(int a1, int a2);
int xnor2(int a1, int a2);
int buff2(int a1, int a2);
int not2(int a1, int a2);
int fanout2(int a1, int a2);

// if NULL is called, there will be segfault. This should never occurs in this program unless error
int (*p[11])(int, int){[0]=nand2,nor2,and2,or2,xor2,xnor2,buff2,not2,NULL, NULL, fanout2};

//-------------------------------------------------


int main(int argc, char* argv[]) {

  // Check the command line input and usage
  if (argc != 4) {
    cout << "Usage: ./logicsim circuit_file input_vectors output_loc" << endl;
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
  //cout << endl;

  
  // Setup the input vector file 
  vector<vector<char> > inputValues;
  ifstream inputStream;
  inputStream.open(argv[2]);
  string inputLine;
  if (!inputStream.is_open()) {
    cout << "ERROR: Cannot read file " << argv[2] << " for input" << endl;
    return 1;
  }

  // Setup the output text file
  ofstream outputStream;
  outputStream.open(argv[3]);
  if (!outputStream.is_open()) {
    cout << "ERROR: Cannot open file " << argv[3] << " for output" << endl;
    return 1;
  }
  
  // Try to read a line of inputs from the file.
  while(getline(inputStream, inputLine)) {
    
    // Clear logic values and detectable faults in my circuit
    myCircuit->clearGateValues();

    // Set new logic values
    myCircuit->setPIValues(constructInputLine(inputLine));

    // Initialize the fault lists for the PIs of the circuit.
    // If the PI == 0, then it can detect sa1; if the PI == 1, it can detect sa0
    vector<Gate*> piGates = myCircuit->getPIGates();	
    for (int i=0; i < piGates.size(); i++) {
      if (piGates[i]->getValue() == LOGIC_ZERO) {
        Fault f = {piGates[i], FAULT_SA1};
        piGates[i]->add_detectableFault(f);
      }
      else if (piGates[i]->getValue() == LOGIC_ONE) {
	     Fault f = {piGates[i], FAULT_SA0};
	     piGates[i]->add_detectableFault(f);
      }
    }

    // ------------------------------------------------------------
    // Add your logic and deductive fault simulation code here.
  
    //first of all we assign values to all outputs 
    vector<Gate*> circuitPOs = myCircuit->getPOGates();
    for(int i = 0; i < circuitPOs.size(); ++i){
        findOutputVal(circuitPOs[i]);
    }

    

    // Stop writing your code here!
    //
    // ---------------------------------------------------------------------------
    // 
    // The following code stores the circuit's output values and detectable faults
    // for each test vector.
    // Do not change any code from here until the end of the main() function.
	
    
    // For each test vector, print the outputs and then the faults detectable at each gate.
    outputStream << "--" << endl;
    vector<Gate*> outputGates = myCircuit->getPOGates();
    for (int i=0; i < outputGates.size(); i++) {
      outputStream << outputGates[i]->printValue();
    }
    outputStream << endl;
      
	  
    for (int i=0; i<myCircuit->getNumberGates(); i++) {
      vector<Fault> v = myCircuit->getGate(i)->get_detectableFaults();
	
      // I am sorting each fault list so the result is independent of the order
      // you insert them into the lists.
      v=sortFaultList(v);
      outputStream << myCircuit->getGate(i)->get_outputName() << ": ";
      outputStream << "{";
    	
      if (v.size() > 1) {		
        for (int j=0; j<v.size()-1; j++) {
          outputStream << v[j].loc->get_outputName() << "/" << (int)(v[j].type) << ", ";
        }
        outputStream << (v[v.size()-1]).loc->get_outputName() << "/" << (int)((v[v.size()-1]).type) << "}" << endl;
      }
      else if (v.size() == 1)  {
        outputStream << v[0].loc->get_outputName() << "/" << (int)(v[0].type) << "}" << endl;
      }    	
      else if (v.size() == 0)
        outputStream << "}" << endl;
    }
  }
  
  // close the input and output streams
  outputStream.close();
  inputStream.close();
  
  
  return 0;
}

// Just used to parse in the values from the 
vector<char> constructInputLine(string line) {
  
  vector<char> inputVals;
  
  for (int i=0; i<line.size(); i++) {
    if (line[i] == '0') 
      inputVals.push_back(LOGIC_ZERO);
    
    else if (line[i] == '1') 
      inputVals.push_back(LOGIC_ONE);

    else if ((line[i] == 'X') || (line[i] == 'x')) {
      cout << "ERROR: Project 2 (faultsim) does not support X inputs." << endl;
      assert(false);		
      //inputVals.push_back(LOGIC_X);
    }

    else if (line[i] == 13) // ignore Windows-style newlines
      ;
   
    else {
      cout << "ERROR: Do not recognize character " << line[i] << " in line " << i+1 << " of input vector file." << endl;
      assert(false);
      //inputVals.push_back(LOGIC_X);
    }
  }
  
  return inputVals;

}

// A function to sort a fault list based on the name of the gate.
// I am sorting the Fault lists before outputting so that every correct answer
// will produce the same faults in the same order.
vector<Fault> sortFaultList(vector<Fault> v) {
  sort(v.begin(), v.end(), faultSortFunc);
  return v;
}

// A helper function for sortFaultList().
bool faultSortFunc(Fault a, Fault b) { 
  return (a.loc->get_outputName() < b.loc->get_outputName());
}


// A compact way to print a fault list. This is just here in case it is helpful for 
// you when debugging your code.
void printFaultList(vector<Fault> l) {
  cout << "{";
  if (l.size() > 1) {		
    for (int i=0; i<l.size()-1; i++) {
      cout << l[i].loc->get_outputName() << "/" << (int)(l[i].type) << ", ";
    }
    cout << (l[l.size()-1]).loc->get_outputName() << "/" << (int)((l[l.size()-1]).type) << "}" << endl;
  }
  else if (l.size() == 1) 
    cout << l[0].loc->get_outputName() << "/" << (int)(l[0].type) << "}" << endl;

  else if (l.size() == 0)
    cout << "}" << endl;
		
}



////////////////////////////////////////////////////////////////////////////
// Place any new functions you add here, between these two bars.

void findOutputVal(Gate* myGate){
    char type = myGate->get_gateType();
    if(type == GATE_PI){
           return;
    }else{
       vector<Gate*> inputGates = myGate->get_gateInputs();
       //calculate values
       int result = 5; 
       for(int i=0; i<inputGates.size(); i++){
           Gate* a = inputGates[i];
           if(a->getValue() == LOGIC_UNSET){
               findOutputVal(a);
           }
           // a function pointer array, use gate type as address to load the corresponding function 
           if(*p[type] == NULL){  
               cout << "ERROR: cannot process gate type "<< myGate->gateTypeName() << endl;
               cout << "Seg Fault occurs here" << endl;
           }
           result = (*p[type])((int)a->getValue(), result);
        }
       //if a gate has a bubble, just run it like a non-inverting gate then load an inverter at the end
       if(type == GATE_NAND || type == GATE_NOR || type == GATE_XNOR){
           result = not2(result, 5);
       }

       myGate->setValue(result);

       //now consider fault list
       
       //get the type first
       switch(type){
           case GATE_NAND:
           case GATE_AND:
               processControlFault(myGate,0);
               break;
           case GATE_NOR:   
           case GATE_OR:   
               processControlFault(myGate,1);
               break;
           case GATE_XOR:
           case GATE_XNOR:   
               processSpecialFault(myGate);
               break;
           case GATE_BUFF:
           case GATE_NOT:   
           case GATE_FANOUT:
               processFlowFault(myGate);
               break;
            default:
               cout << "ERROR: cannot process gate type "<< myGate->gateTypeName() << endl;
       }
    }
}

// used for and, nand, or, nor
void processControlFault(Gate* gate, int conVal){ 
    int flag_a = 0, flag_b = 0; // flag_a indicates control inputs, flag_b indicate noncontrol input
    vector<Gate*> in = gate->get_gateInputs();
    vector<Fault> conFault, nonFault, finFault; //controlling value fault list, noncontrolling value fault list, final fault list
    for(int i=0; i<in.size(); i++) {
        Gate* a = in[i];
        //cout << a->get_outputName() << ": " << a->printValue() << endl;
        //printFaultList(a->get_detectableFaults());
        if(a->getValue() == conVal) {          // if control input, use it for intersection with other control inputs
            if(!flag_a && conFault.size() == 0) {        //if flag not set and fault list is empty, it is the first control input detected
                conFault = a->get_detectableFaults();       //first control input only
            }else {
                conFault = faultIntersect(conFault, a->get_detectableFaults());      
            }
            flag_a = 1; // set the flag 
        } //end of conVal evaluation 
        else {
            if(!flag_b && nonFault.size() == 0) {
                nonFault = a->get_detectableFaults();
            }else {
                nonFault = faultUnion(nonFault, a->get_detectableFaults());
            }
            flag_b = 1;
        }// end of non-conVal evalation
    } // end of fault list loop

    //determine the expression to use
    if(flag_a) {
        //controlling input exists
        finFault = faultMinus(conFault, nonFault); //con - noncon 
    } else {
        //no controlling input
        finFault = nonFault;
    }

    /*
    cout << gate->get_outputName() << ": ";
    printFaultList(conFault);
    printFaultList(nonFault);
    printFaultList(finFault);
    */

    // set final fault list to this gate
    gate->set_detectableFaults(finFault);

    //add output fault to the list
    Fault f = {gate, char(gate->getValue()?FAULT_SA0:FAULT_SA1)};
    gate->add_detectableFault(f);

}

//use for xor, xnor 
void processSpecialFault(Gate* gate){ 
    vector<Gate*> in = gate->get_gateInputs();
    vector<Fault> finFault; //controlling value fault list, noncontrolling value fault list, final fault list
    finFault = in[0]->get_detectableFaults();
    //simple idea, initial list is in[0]. Starting with in[1], if a fault occurs in both in[0] and in[1], then it occurs even times, we don't need it
    //the new fault list will not contain this fault. But if we detect it again in the future, it will be detectable, symmetric difference will add it
    for(int i=1; i<in.size();i++){
        Gate* a = in[i];
        finFault = faultSymDiff(finFault,a->get_detectableFaults());
    }

    // set final fault list to this gate
    gate->set_detectableFaults(finFault);

    //add output fault to the list
    Fault f = {gate, char(gate->getValue()?FAULT_SA0:FAULT_SA1)};
    gate->add_detectableFault(f);
}

//use for branch, buffer, inverter
void processFlowFault(Gate* gate){
    vector<Gate*> in = gate->get_gateInputs();
    vector<Fault> finFault;

    // this type of gate has only 1 inputs, use in[0]'s fault list as the base
    finFault = in[0]->get_detectableFaults();

    // set final fault list to this gate
    gate->set_detectableFaults(finFault);

    //add output fault to the list
    Fault f = {gate, char(gate->getValue()?FAULT_SA0:FAULT_SA1)};
    gate->add_detectableFault(f);
}


// union of two fault lists
vector<Fault> faultUnion(vector<Fault>a, vector<Fault>b){
    vector<Fault> dest;
    //sort first 
    a = sortFaultList(a);
    b = sortFaultList(b);
    //union
    set_union(a.begin(), a.end(),
            b.begin(), b.end(),
            back_inserter(dest), faultSortFunc);
    return dest;
}
// intersection of two fault lists
vector<Fault> faultIntersect(vector<Fault>a, vector<Fault>b){
    vector<Fault> dest;
    //sort first 
    a = sortFaultList(a);
    b = sortFaultList(b);
    //intersection
    set_intersection(a.begin(), a.end(),
            b.begin(), b.end(),
            back_inserter(dest), faultSortFunc);
    return dest;
}
// fault list a - fault list b
vector<Fault> faultMinus(vector<Fault>a, vector<Fault>b){
    vector<Fault> dest;
    //sort first 
    a = sortFaultList(a);
    b = sortFaultList(b);
    //difference
    set_difference(a.begin(), a.end(),
            b.begin(), b.end(),
            back_inserter(dest), faultSortFunc);
    return dest;
}
// the opposite of intersection: all faults that in either a or b, but not in both
vector<Fault> faultSymDiff(vector<Fault>a, vector<Fault>b){
    vector<Fault> dest;
    //sort first 
    a = sortFaultList(a);
    b = sortFaultList(b);
    //difference
    set_symmetric_difference(a.begin(), a.end(),
            b.begin(), b.end(),
            back_inserter(dest), faultSortFunc);
    return dest;

}

//To calculate output at each gate
int nand2(int a1, int a2){
    return and2(a1,a2);
}

int nor2(int a1, int a2){
    return or2(a1,a2);
}

int and2(int a1, int a2){
    if(a2 == 5) return a1;
    else if(a1 == 0 || a2 == 0) return 0;
    else return 1;
}

int or2(int a1, int a2){
    if(a2 == 5) return a1;
    else if(a1 == 1 || a2 == 1) return 1;
    else return 0;
}

int xnor2(int a1, int a2){
    return xor2(a1,a2);
}

int buff2(int a1, int a2){
    return a1;
}

int xor2(int a1, int a2){
    if(a2 == 5) return a1;
    else if(a1 == a2) return 0;
    else return 1;
}

int not2(int a1, int a2){
    if(a1 == 0) return 1;
    else return 0;
}

int fanout2(int a1, int a2){
    return a1;
}

////////////////////////////////////////////////////////////////////////////


