// ESE-549 Project 3 Handout
// Peter Milder

/** @file */

// For you to do:
// Part 1: 
//    - Write the getObjective() function
//    - Write the updateDFrontier() function
//    - Write the backtrace() function
//    - Write the podemRecursion() function
// Then your basic PODEM implementation should be finished.
// Test this code carefully.
// Note you can also turn on a "checkTest" mode that will use your 
// simulator to run the test after you generated it and check that it
// correctly detects the faults.
//
// Part 2:
//    - Write the eventDrivenSim() function.
//    - Change the simulation calls in your podemRecursion() function
//      from using the old simFullCircuit() function to the new
//      event-driven simulator.
// Then, your PODEM implementation should run considerably faster
// (probably 8 to 10x faster for large circuits).
// Test everything and then evaluate the speedup.
// A quick way to figure out how long it takes to run something in
// Linux is:
//   time ./atpg ... type other params...
//
// Part 3:
//    - Apply optimizations as discussed in project description


#include <iostream>
#include <fstream> 
#include <vector>
#include <queue>
#include <time.h>
#include <stdio.h>
#include "parse_bench.tab.h"
#include "ClassCircuit.h"
#include "ClassGate.h"
#include <limits>
#include <stdlib.h>
#include <time.h>

using namespace std;

/** DEBUG MACRO FOR STUDENT **/
//#define XOR 1
//#define DF 1



/**  @brief Just for the parser. Don't touch. */
extern "C" int yyparse();

/**  Input file for parser. Don't touch. */
extern FILE *yyin;

/** Our circuit. We declare this external so the parser can use it more easily. */
extern Circuit* myCircuit;

//--------------------------
// Helper functions
void printUsage();
vector<char> constructInputLine(string line);
bool checkTest(Circuit* myCircuit);
string printPIValue(char v);
//--------------------------

//----------------------------
// Functions for logic simulation
void simFullCircuit(Circuit* myCircuit);
void simGateRecursive(Gate* g);
void eventDrivenSim(Circuit* myCircuit, queue<Gate*> q);
char simGate(Gate* g);
char evalGate(vector<char> in, int c, int i);
char EvalXORGate(vector<char> in, int inv);
int LogicNot(int logicVal);
void setValueCheckFault(Gate* g, char gateValue);
//-----------------------------

//----------------------------
// Functions for PODEM:
bool podemRecursion(Circuit* myCircuit);
bool getObjective(Gate* &g, char &v, Circuit* myCircuit);
void updateDFrontier(Circuit* myCircuit);
void backtrace(Gate* &pi, char &piVal, Gate* objGate, char objVal, Circuit* myCircuit);
//--------------------------

///////////////////////////////////////////////////////////
// Global variables
// These are made global to make your life slightly easier.

/** Global variable: a vector of Gate pointers for storing the D-Frontier. */
vector<Gate*> dFrontier;

/** Global variable: holds a pointer to the gate with stuck-at fault on its output location. */
Gate* faultLocation;     

/** Global variable: holds the logic value you will need to activate the stuck-at fault. */
char faultActivationVal;

///////////////////////////////////////////////////////////

// A struct we will use to represent a fault.
// loc is a pointer to the gate output where this fault is located
// val is 0 or 1 for SA0 or SA1
struct faultStruct {
  Gate* loc;
  char val;
};


/** @brief The main function.
 * 
 * You do not need to change anything in the main function,
 * although you should understand what is happening
 * here.
 */
int main(int argc, char* argv[]) {

  // Check the command line input and usage
  if (argc != 5) {
    printUsage();    
    return 1;
  }
  
  int mode = atoi(argv[1]);
  if ((mode < 0) || (mode > 1)) {
    printUsage();    
    return 1;   
  }


  // Parse the bench file and initialize the circuit. (Using C style for our parser.)
  FILE *benchFile = fopen(argv[2], "r");
  if (benchFile == NULL) {
    cout << "ERROR: Cannot read file " << argv[2] << " for input" << endl;
    return 1;
  }
  yyin=benchFile;
  yyparse();
  fclose(benchFile);

  myCircuit->setupCircuit(); 
  cout << endl;

  // Setup the output text file
  ofstream outputStream;
  outputStream.open(argv[3]);
  if (!outputStream.is_open()) {
    cout << "ERROR: Cannot open file " << argv[3] << " for output" << endl;
    return 1;
  }
   
  // Open the fault file.
  ifstream faultStream;
  string faultLocStr;
  faultStream.open(argv[4]);
  if (!faultStream.is_open()) {
    cout << "ERROR: Cannot open fault file " << argv[4] << " for input" << endl;
    return 1;
  }
  
  // This vector will hold all of the faults you want to generate tests for. 
  // For Parts 1 and 2, you will simply go through this vector in order. 
  vector<faultStruct> faultList;

  // Go through each lone in the fault file, and add the faults to faultList.
  while(getline(faultStream, faultLocStr)) {
    string faultTypeStr;
      
    if (!(getline(faultStream, faultTypeStr))) {
      break;
    }
      
    char faultType = atoi(faultTypeStr.c_str());
    Gate* fl = myCircuit->findGateByName(faultLocStr);

    faultStruct fs = {fl, faultType};
    faultList.push_back(fs);
  }
  faultStream.close();

  if (mode == 0) {   // mode 0 means the system will run PODEM for each fault in the faultList.
      for (int faultNum = 0; faultNum < faultList.size(); faultNum++) {

       // Clear the old fault
       myCircuit->clearFaults();
         
       // set up the fault we are trying to detect
       faultLocation = faultList[faultNum].loc;
       char faultType = faultList[faultNum].val;
       faultLocation->set_faultType(faultType);      
       faultActivationVal = (faultType == FAULT_SA0) ? LOGIC_ONE : LOGIC_ZERO;
         
       // set all gate values to X
       for (int i=0; i < myCircuit->getNumberGates(); i++) {
         myCircuit->getGate(i)->setValue(LOGIC_X);
       }

       // initialize the D frontier.
       dFrontier.clear();
       
       // call PODEM recursion function
       bool res = podemRecursion(myCircuit);

       // If we succeed, print the test we found to the output file.
       if (res == true) {
         vector<Gate*> piGates = myCircuit->getPIGates();
         for (int i=0; i < piGates.size(); i++)
           outputStream << printPIValue(piGates[i]->getValue());
         outputStream << endl;
       }

       // If we failed to find a test, print a message to the output file
       else {
         outputStream << "none found" << endl;
       }

       // Lastly, you can use this to test that your PODEM-generated test
       // correctly detects the already-set fault.
       // Of course, this assumes that your simulation code is correct.

       // Comment this out when you are evaluating the runtime of your
       // ATPG system because it will add extra time.
    /* 
       if (res == true) {
         if (!checkTest(myCircuit)) {
           cout << "ERROR: PODEM returned true, but generated test does not detect fault on PO." << endl;
           myCircuit->printAllGates();
           assert(false);
         }
       }
*/
       // Just printing to screen to let you monitor progress. You can comment this out if you want.       
       cout << "Fault = " << faultLocation->get_outputName() << " / " << (int)(faultType) << ";";
       if (res == true)
         cout << " test found; " << endl;
       else
         cout << " no test found; " << endl;
     }
  }
  else {   // mode 1 means that you are trying to optimize the test set size (Part 3). Add your code here
      // TODOP3: Write this. See Part 3
      // Optimize fault list
        
      srand(time(NULL));
      for (int faultNum = 0; faultNum < faultList.size(); faultNum++) {

       // Clear the old fault
       myCircuit->clearFaults();
         
       // set up the fault we are trying to detect
       faultLocation = faultList[faultNum].loc;
       char faultType = faultList[faultNum].val;
       faultLocation->set_faultType(faultType);      
       faultActivationVal = (faultType == FAULT_SA0) ? LOGIC_ONE : LOGIC_ZERO;
         
       // set all gate values to X
       for (int i=0; i < myCircuit->getNumberGates(); i++) {
         myCircuit->getGate(i)->setValue(LOGIC_X);
       }

       // initialize the D frontier.
       dFrontier.clear();
       
       // call PODEM recursion function
       bool res = podemRecursion(myCircuit);

       // If we succeed, print the test we found to the output file.
       if (res == true) {
           vector<Gate*> piGates = myCircuit->getPIGates();
           for (int i=0; i < piGates.size(); i++){
               // randomly fill in 1 or 0 for inputs with value X. This may result in uncertain test vector size
               char inVal = piGates[i]->getValue();
               if(inVal == LOGIC_X){
                   if((rand()%10) < 5){
                       inVal = LOGIC_ZERO;
                   }else{
                       inVal = LOGIC_ONE;
                   }
               }
               outputStream << printPIValue(inVal);
               piGates[i]->setValue(inVal);
           }

           //comment out this if you don't want number of faults count next to the test vector output
           outputStream << "  ";

            cout << "Fault = " << faultLocation->get_outputName() << " / " << (int)(faultType) << ";";
            cout << " test found; " << endl;

            //simulate for other faults

            int count = 1;
           for(int faultCursor = faultNum+1; faultCursor < faultList.size(); faultCursor++){
               myCircuit->clearFaults();
               faultLocation = faultList[faultCursor].loc;
               faultType = faultList[faultCursor].val;
               faultLocation->set_faultType(faultType);

               //reset PI
               for(int i=0; i<piGates.size();i++){
                   char gateVal;
                   switch(piGates[i]->getValue()){
                       case LOGIC_ONE:
                       case LOGIC_D:
                           gateVal = LOGIC_ONE;
                           break;
                       case LOGIC_ZERO:
                       case LOGIC_DBAR:
                           gateVal = LOGIC_ZERO;
                           break;
                       default:
                           cerr << "don't know how to reset PI values" << endl;
                           assert(false);
                   }
                   setValueCheckFault(piGates[i], gateVal);
               }

               bool simRes = checkTest(myCircuit);
               if(simRes){
                   count++;
                   cout << "Fault = " << faultLocation->get_outputName() << " / " << (int)(faultType) << ";";
                   cout << " test also found; " << endl;
                   faultList.erase(faultList.begin()+faultCursor);
               }
           }
           //comment out this if you don't want number of faults count next to the test vector output
               outputStream << count << endl;
       }
       
       // If we failed to find a test, print a message to the output file
       else {
           cout << "Fault = " << faultLocation->get_outputName() << " / " << (int)(faultType) << ";";
           cout << " no test found; " << endl;
           outputStream << "none found" << endl;
       }
      }
  }
  
  // close the output and fault streams
  outputStream.close();

    
  return 0;
}


/////////////////////////////////////////////////////////////////////
// Functions in this section are helper functions.
// You should not need to change these, except if you want
// to enable the checkTest function (which will use your simulator
// to attempt to check the test vector computed by PODEM.)


/** @brief Print usage information (if user provides incorrect input).
 * 
 * You don't need to touch this.
 */
void printUsage() {
  cout << "Usage: ./atpg [mode] [bench_file] [output_loc] [fault_file]" << endl << endl;
  cout << "   mode:          0 for normal mode, 1 to minimize test set size" << endl;
  cout << "   bench_file:    the target circuit in .bench format" << endl;
  cout << "   output_loc:    location for output file" << endl;
  cout << "   fault_file:    faults to be considered" << endl;
  cout << endl;
  cout << "   The system will generate a test pattern for each fault listed" << endl;
  cout << "   in fault_file and store the result in output_loc." << endl;
  cout << endl;	
}


// You don't need to touch this.
/** @brief Used to parse in the values from the input file.
 * 
 * You don't need to touch this.
 */
vector<char> constructInputLine(string line) {
  
  vector<char> inputVals;
  
  for (int i=0; i<line.size(); i++) {
    if (line[i] == '0') 
      inputVals.push_back(LOGIC_ZERO);
    
    else if (line[i] == '1') 
      inputVals.push_back(LOGIC_ONE);

    else if ((line[i] == 'X') || (line[i] == 'x')) {
      inputVals.push_back(LOGIC_X);
    }
   
    else {
      cout << "ERROR: Do not recognize character " << line[i] << " in line " << i+1 << " of input vector file." << endl;
      assert(false);
      //inputVals.push_back(LOGIC_X);
    }
  }  
  return inputVals;
}

/** @brief Uses your simulator to check validity of your test.
 * 
 * This function gets called after your PODEM algorithm finishes.
 * If you enable this, it will clear the circuit's internal values,
 * and re-simulate the vector PODEM found to test your result.
 
 * This is helpful when you are developing and debugging, but will just
 * slow things down once you know things are correct.
 
 * This function of course assumes that your simulation code 
 * is correct.
*/
bool checkTest(Circuit* myCircuit) {

  simFullCircuit(myCircuit);

  // look for D or D' on an output
  vector<Gate*> poGates = myCircuit->getPOGates();
  for (int i=0; i<poGates.size(); i++) {
    char v = poGates[i]->getValue();
    if ((v == LOGIC_D) || (v == LOGIC_DBAR)) {
      return true;
    }
  }

  // If we didn't find D or D' on any PO, then our test was not successful.
  return false;

}

/** @brief Prints a PI value. 
 * 
 * This is just a helper function used when storing the final test you computed.
 *  You don't need to run or modify this.
 */
string printPIValue(char v) {
  switch(v) {
  case LOGIC_ZERO: return "0";
  case LOGIC_ONE: return "1";
  case LOGIC_UNSET: return "U";
  case LOGIC_X: return "X";
  case LOGIC_D: return "1";
  case LOGIC_DBAR: return "0";
  }
  return "";
}

// end of helper functions
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Start of functions for circuit simulation.


/** @brief Runs full circuit simulation
 *
 * Full-circuit simulation: set all non-PI gates to LOGIC_UNSET
 * and call the recursive simulate function on all PO gates.
 * Don't change this function unless you want to adapt it to use your Project 1 code.
 */
void simFullCircuit(Circuit* myCircuit) {
  for (int i=0; i<myCircuit->getNumberGates(); i++) {
    Gate* g = myCircuit->getGate(i);
    if (g->get_gateType() != GATE_PI)
      g->setValue(LOGIC_UNSET);      
  }  
  vector<Gate*> circuitPOs = myCircuit->getPOGates();
  for (int i=0; i < circuitPOs.size(); i++) {
    simGateRecursive(circuitPOs[i]);
  }
}



// Recursive function to find and set the value on Gate* g.
// This function calls simGate and setValueCheckFault. 
// Don't change this function.
/** @brief Recursive function to find and set the value on Gate* g.
 * \param g The gate to simulate.
 * This function prepares Gate* g to to be simulated by recursing
 * on its inputs (if needed).
 * 
 * Then it calls \a simGate(g) to calculate the new value.
 * 
 * Lastly, it will set the Gate's output value based on
 * the calculated value.
 * 
 * \note Do not change this function. 
 */
void simGateRecursive(Gate* g) {

  // If this gate has an already-set value, you are done.
  if (g->getValue() != LOGIC_UNSET)
    return;
  
  // Recursively call this function on this gate's predecessors to
  // ensure that their values are known.
  vector<Gate*> pred = g->get_gateInputs();
  for (int i=0; i<pred.size(); i++) {
    simGateRecursive(pred[i]);
  }
  
  char gateValue = simGate(g);

  // After I have calculated this gate's value, check to see if a fault changes it and set.
  setValueCheckFault(g, gateValue);
}


/** @brief Perform event-driven simulation.
 * \note You will write this function in Part 2.
 * 
 * Please see the project handout for a description of what
 * we are doing here and why.

 * This function takes as input the Circuit* and a queue<Gate*>
 * indicating the remaining gates that need to be evaluated.
 */
void eventDrivenSim(Circuit* myCircuit, queue<Gate*> q) {
    Gate *g1;
    char gateVal;
    vector<Gate*> gates;


    while(!q.empty()){
        g1 = q.front();
        q.pop();
        gateVal = simGate(g1);
#ifdef DF
        int dFlag = 0;   //flag showing that a gate is D frontier
        if(gateVal == LOGIC_X){
            vector<Gate*> inGates = g1->get_gateInputs();
            for(int i=0; i<inGates.size(); i++){
                if((inGates[i]->getValue() == LOGIC_D) || (inGates[i]->getValue() == LOGIC_DBAR)){
                    dFlag = 1;
                    break;
                }
            }
        }

        int found = 0; //flag showing whether this gate is in the DFrontier vector
        int num; 
        for(num = 0; num < dFrontier.size(); num++){
            if(dFrontier[num] == g1){
                found = 1;
                break;
            }
        }
        if(dFlag){
            if(!found){
                dFrontier.push_back(g1); //D Froniter not found in the vector should be added to it
            }
        }else{
            if(found){
                dFrontier.erase(dFrontier.begin()+num); //no longer a D Frontier, remove it
            }
        }

#endif        
        // if no change, move to next gate
        if(gateVal == g1->getValue()){
            continue;
        }
        //if differ, set output values and push all output gates to queue
        setValueCheckFault(g1, gateVal);
        gates = g1->get_gateOutputs();
        for(int i=0; i < gates.size(); i++){
            q.push(gates[i]);
        }

    }
}


/** @brief Simulate the value of the given Gate.
 *
 * This is a gate simulation function -- it will simulate the gate g
 * with its current input values and return the output value.
 * This function does not deal with the fault. (That comes later.)
 * \note You do not need to change this function.
 *
 */
char simGate(Gate* g) {
  // For convenience, create a vector of the values of this
  // gate's inputs.
  vector<Gate*> pred = g->get_gateInputs();
  vector<char> inputVals;   
  for (int i=0; i<pred.size(); i++) {
    inputVals.push_back(pred[i]->getValue());      
  }

  char gateType = g->get_gateType();
  char gateValue;
  // Now, set the value of this gate based on its logical function and its input values
  switch(gateType) {   
  case GATE_NAND: { gateValue = evalGate(inputVals, 0, 1); break; }
  case GATE_NOR: { gateValue = evalGate(inputVals, 1, 1); break; }
  case GATE_AND: { gateValue = evalGate(inputVals, 0, 0); break; }
  case GATE_OR: { gateValue = evalGate(inputVals, 1, 0); break; }
  case GATE_BUFF: { gateValue = inputVals[0]; break; }
  case GATE_NOT: { gateValue = LogicNot(inputVals[0]); break; }
  case GATE_XOR: { gateValue = EvalXORGate(inputVals, 0); break; }
  case GATE_XNOR: { gateValue = EvalXORGate(inputVals, 1); break; }
  case GATE_FANOUT: {gateValue = inputVals[0]; break; }
  default: { cout << "ERROR: Do not know how to evaluate gate type " << gateType << endl; assert(false);}
  }    

  return gateValue;
}


/** @brief Evaluate a NAND, NOR, AND, or OR gate.
 * \param in The logic value's of this gate's inputs.
 * \param c The controlling value of this gate type (e.g. c==0 for an AND or NAND gate)
 * \param i The inverting value for this gate (e.g. i==0 for AND and i==1 for NAND)
 * \returns The logical value produced by this gate (not including a possible fault on this gate).
 * \note You do not need to change this function.
 */
char evalGate(vector<char> in, int c, int i) {

  // Are any of the inputs of this gate the controlling value?
  bool anyC = find(in.begin(), in.end(), c) != in.end();
  
  // Are any of the inputs of this gate unknown?
  bool anyUnknown = (find(in.begin(), in.end(), LOGIC_X) != in.end());

  int anyD    = find(in.begin(), in.end(), LOGIC_D)    != in.end();
  int anyDBar = find(in.begin(), in.end(), LOGIC_DBAR) != in.end();


  // if any input is c or we have both D and D', then return c^i
  if ((anyC) || (anyD && anyDBar))
    return (i) ? LogicNot(c) : c;
  
  // else if any input is unknown, return unknown
  else if (anyUnknown)
    return LOGIC_X;

  // else if any input is D, return D^i
  else if (anyD)
    return (i) ? LOGIC_DBAR : LOGIC_D;

  // else if any input is D', return D'^i
  else if (anyDBar)
    return (i) ? LOGIC_D : LOGIC_DBAR;

  // else return ~(c^i)
  else
    return LogicNot((i) ? LogicNot(c) : c);
}

/** @brief Evaluate an XOR or XNOR gate.
 * \param in The logic value's of this gate's inputs.
 * \param inv The inverting value for this gate (e.g. i==0 for XOR and i==1 for XNOR)
 * \returns The logical value produced by this gate (not including a possible fault on this gate).
 * \note You do not need to change this function.
 */
char EvalXORGate(vector<char> in, int inv) {

  // if any unknowns, return unknown
  bool anyUnknown = (find(in.begin(), in.end(), LOGIC_X) != in.end());
  if (anyUnknown)
    return LOGIC_X;

  // Otherwise, let's count the numbers of ones and zeros for faulty and fault-free circuits.
  // This is not required for your project, but this will with with XOR and XNOR with > 2 inputs.
  int onesFaultFree = 0;
  int onesFaulty = 0;

  for (int i=0; i<in.size(); i++) {
    switch(in[i]) {
    case LOGIC_ZERO: {break;}
    case LOGIC_ONE: {onesFaultFree++; onesFaulty++; break;}
    case LOGIC_D: {onesFaultFree++; break;}
    case LOGIC_DBAR: {onesFaulty++; break;}
    default: {cout << "ERROR: Do not know how to process logic value " << in[i] << " in Gate::EvalXORGate()" << endl; return LOGIC_X;}
    }
  }
  
  int XORVal;

  if ((onesFaultFree%2 == 0) && (onesFaulty%2 ==0))
    XORVal = LOGIC_ZERO;
  else if ((onesFaultFree%2 == 1) && (onesFaulty%2 ==1))
    XORVal = LOGIC_ONE;
  else if ((onesFaultFree%2 == 1) && (onesFaulty%2 ==0))
    XORVal = LOGIC_D;
  else
    XORVal = LOGIC_DBAR;

  return (inv) ? LogicNot(XORVal) : XORVal;

}


/** @brief Perform a logical NOT operation on a logical value using the LOGIC_* macros
 * \note You will not need to modify this function.
 */
int LogicNot(int logicVal) {
  if (logicVal == LOGIC_ONE)
    return LOGIC_ZERO;
  if (logicVal == LOGIC_ZERO)
    return LOGIC_ONE;
  if (logicVal == LOGIC_D)
    return LOGIC_DBAR;
  if (logicVal == LOGIC_DBAR)
    return LOGIC_D;
  if (logicVal == LOGIC_X)
    return LOGIC_X;
      
  cout << "ERROR: Do not know how to invert " << logicVal << " in LogicNot(int logicVal)" << endl;
  return LOGIC_UNSET;
}

/** @brief Set the value of Gate* g to value gateValue, accounting for any fault on g.
    \note You will not need to modify this.
 */
void setValueCheckFault(Gate* g, char gateValue) {
  if ((g->get_faultType() == FAULT_SA0) && (gateValue == LOGIC_ONE)) 
  	g->setValue(LOGIC_D);
  else if ((g->get_faultType() == FAULT_SA0) && (gateValue == LOGIC_DBAR)) 
  	g->setValue(LOGIC_ZERO);
  else if ((g->get_faultType() == FAULT_SA1) && (gateValue == LOGIC_ZERO)) 
  	g->setValue(LOGIC_DBAR);
  else if ((g->get_faultType() == FAULT_SA1) && (gateValue == LOGIC_D)) 
  	g->setValue(LOGIC_ONE);
  else
  	g->setValue(gateValue);
}

// End of functions for circuit simulation
////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////
// Begin functions for PODEM.

// TODOP1
/** @brief PODEM recursion.
 *
 * \note For Part 1, you must write this, following the pseudocode from class. 
 * Make use of the getObjective and backtrace functions.
 * For "imply" use the simulation. In Part 1, just call simFullCircuit (after setting
 * the PI to the appropriate value). In Part 2, replace this with a call to launch 
 * your event-driven simulator.
 */
bool podemRecursion(Circuit* myCircuit) {
    Gate *g, *pi ;
    char v, piVal;
    vector<Gate*> gates;
    queue<Gate*> q;
    //check whether a D/D_BAR exists in output gates
    for(int i = 0; i < myCircuit->getNumberPOs(); i++){
        g = myCircuit->getPOGates()[i];
        if(g->getValue() == LOGIC_D || g->getValue() == LOGIC_DBAR){
            return true;
        }
    }
    if(!getObjective(g, v, myCircuit)){
        return false;
    }


    backtrace(pi, piVal, g, v, myCircuit);

    // queue contains all pi's output gates
    gates = pi->get_gateOutputs();
    for(int i=0; i<gates.size();i++){
        q.push(gates[i]);
    }

    setValueCheckFault(pi, piVal);
    //simFullCircuit(myCircuit);
    eventDrivenSim(myCircuit,q);
#ifndef DF
    updateDFrontier(myCircuit);
#endif
    if(podemRecursion(myCircuit)){
        return true;
    }

    setValueCheckFault(pi, LogicNot(piVal));
    //simFullCircuit(myCircuit);
    eventDrivenSim(myCircuit,q); 
#ifndef DF
    updateDFrontier(myCircuit);
#endif
    if(podemRecursion(myCircuit)){
        return true;
    }

    pi->setValue(LOGIC_X);
    //simFullCircuit(myCircuit);
    eventDrivenSim(myCircuit,q); 
#ifndef DF
    updateDFrontier(myCircuit);
#endif
    return false;
}

// Find the objective for myCircuit. The objective is stored in g, v.
// 
// TODOP1: Write this function, based on the pseudocode from
// class or your textbook.
/** @brief PODEM objective function.
 *  \param g Use this pointer to store the objective Gate your function picks.
 *  \param v Use this char to store the objective value your function picks.
 *  \returns True if the function is able to determine an objective, and false if it fails.
 * \note For Part 1, you must write this, following the pseudocode in class and the code's comments.
 */
bool getObjective(Gate* &g, char &v, Circuit* myCircuit) {
    int i = 0;
    Gate* d;
    vector<Gate*> inputGates;

    // if not excited, excite the fault
    if(faultLocation->getValue() == LOGIC_X){
        g = faultLocation;
        v = faultActivationVal;
        return true;
    }else if(!(faultLocation->getValue() == LOGIC_D) && !(faultLocation->getValue() == LOGIC_DBAR)){
        //fault not excite, but faultLocation output is defined
        return false;
    }

    // no need to update because if a fault has been excited, either updateDFrontier or eventDrivenSim must have been called as well
    //updateDFrontier(myCircuit);
    if(dFrontier.empty()){
        return false;
    }

    d = dFrontier[0];
    inputGates = d->get_gateInputs();
    //advance to the first LOGIC_X gate, if program stucks, this must be the reason
    while(inputGates[i]->getValue() != LOGIC_X)
        i++;
    g = inputGates[i];

    switch(d->get_gateType()){
        case GATE_NAND:
        case GATE_AND:
            v = LOGIC_ONE;
            break;
        case GATE_NOR:
        case GATE_OR:
        case GATE_XOR:
        case GATE_XNOR:
            v = LOGIC_ZERO;
            break;
        default:
            cerr << "getObjective d frontier gate type error." << endl;
            assert(false);

    }
    return true;


  // First you will need to check if the fault is activated yet.
  // Note that in the setup above we set up a global variable
  // Gate* faultLocation which represents the gate with the stuck-at
  // fault on its output. Use that when you check if the fault is
  // excited.

  // Another note: if the fault is not excited but the fault 
  // location value is not X, then we have failed to activate 
  // the fault. In this case getObjective should fail and Return false.  
  // Otherwise, use the D-frontier to find an objective.

  // Don't forget to call updateDFrontier to make sure your dFrontier (a global variable)
  // is up to date.

  // Remember, for Parts 1/2 if you want to match my reference solution exactly,
  // you should choose the first gate in the D frontier (dFrontier[0]), and pick
  // its first approrpriate input.
  
  // If you can successfully set an objective, return true.

}


// A very simple method to update the D frontier.
// TODOP1: Write this code based on the pseudocode below.
/** @brief A simple method to compute the set of gates on the D frontier.
 *
 * \note For Part 1, you must write this. The simplest form follows the pseudocode included in the comments.
 */
void updateDFrontier(Circuit* myCircuit) {

    Gate *g1, *g2;
    vector<Gate*> gates;
    dFrontier.clear();
    for (int i=0; i<myCircuit->getNumberGates(); i++) {
        g1 = myCircuit->getGate(i);
        if(g1->getValue() != LOGIC_X){
            continue;
        }
        gates = g1->get_gateInputs();
        for(int j = 0; j < gates.size(); j++){
            g2 = gates[j];
            if(g2->getValue() == LOGIC_D || g2->getValue() == LOGIC_DBAR){
                dFrontier.push_back(g1);
                break;
            }
        }
    }
  // Procedure:
  //  - clear the dFrontier vector (stored as the global variable dFrontier -- see the top of the file)
  //  - loop over all gates in the circuit; for each gate, check if it should be on D-frontier; if it is,
  //    add it to the dFrontier vector.

  // One way to improve speed for Part 3 would be to improve D-frontier management. You can add/remove
  // gates from the D frontier during simulation, instead of adding an entire pass over all the gates
  // like this.

}
#ifdef XOR
// Backtrace: given objective objGate and objVal, then figure out which input (pi) to set 
// and which value (piVal) to set it to.
// TODOP1: write this

/** @brief PODEM backtrace function
 * \param pi Output: A Gate pointer to the primary input your backtrace function found.
 * \param piVal Output: The value you want to set that primary input to
 * \param objGate Input: The objective Gate (computed by getObjective)
 * \param objVal Input: the objective value (computed by getObjective)
 * \note Write this function based on the psuedocode from class.
 */
void backtrace(Gate* &pi, char &piVal, Gate* objGate, char objVal, Circuit* myCircuit) {
    int num_inversion = 0, i, j, jFlag, inv, debug;
    vector<Gate*> inputGates;
    vector<char> inputVals;
    char type;
    pi = objGate;

    while((type = pi->get_gateType()) != GATE_PI){
        inputGates = pi->get_gateInputs();
        i = 0;
        while(inputGates[i]->getValue() != LOGIC_X)
            i++;
        pi = inputGates[i];
        j = i+1;
        jFlag = 0; // After we find a LOGIC_X in XOR inputs, we look for other Xs. If not exist, the previous input is the only unknown
        inv = 0;
        switch(type){
            case GATE_NOR:
            case GATE_NAND:
            case GATE_NOT:
                num_inversion++;
                break;
            case GATE_XNOR:
                inv = 1;
            case GATE_XOR:
                while(j < inputGates.size()){
                    if(inputGates[j]->getValue() == LOGIC_X){
                        jFlag = 1;
                    }
                    j++;
                }
                // if not a unique X input, we follow standard precedure: xnor is inverting, xor is non-inverting
                if(jFlag){
                    num_inversion+= inv;
                }else{
                    for(int num = 0; num < inputGates.size(); num++){
                        if(num != i){
                            inputVals.push_back(inputGates[num]->getValue());
                        }
                    }// find out what other input values are, 1 is inverting, 0 is non-inverting
                    if((debug = EvalXORGate(inputVals, inv)) == LOGIC_ONE)
                        num_inversion++;
                }
        }
    }

    if(num_inversion%2){
        piVal = LogicNot(objVal);
    }else{
        piVal = objVal;
    }


}
#else
// Backtrace: given objective objGate and objVal, then figure out which input (pi) to set 
// and which value (piVal) to set it to.
// TODOP1: write this

/** @brief PODEM backtrace function
 * \param pi Output: A Gate pointer to the primary input your backtrace function found.
 * \param piVal Output: The value you want to set that primary input to
 * \param objGate Input: The objective Gate (computed by getObjective)
 * \param objVal Input: the objective value (computed by getObjective)
 * \note Write this function based on the psuedocode from class.
 */
void backtrace(Gate* &pi, char &piVal, Gate* objGate, char objVal, Circuit* myCircuit) {
    int num_inversion = 0, i;
    vector<Gate*> inputGates;
    char type;
    pi = objGate;

    while((type = pi->get_gateType()) != GATE_PI){
        inputGates = pi->get_gateInputs();
        i = 0;
        while(inputGates[i]->getValue() != LOGIC_X)
            i++;
        pi = inputGates[i];
        switch(type){
            case GATE_XNOR:
            case GATE_NOR:
            case GATE_NAND:
            case GATE_NOT:
                num_inversion++;
                break;
        }
    }

    if(num_inversion%2){
        piVal = LogicNot(objVal);
    }else{
        piVal = objVal;
    }

}
#endif
////////////////////////////////////////////////////////////////////////////
// Please place any new functions you add here, between these two bars.



////////////////////////////////////////////////////////////////////////////



