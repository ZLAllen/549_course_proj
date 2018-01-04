/** \class Gate
 * \brief A Boolean logic gate, with pointers to its input sources and destinations of its outputs.
 * 
 * A \a Gate is a data structure for storing the information about a gate. It contains a number of 
 * useful things:
 * - A variable which indicates what type of gate it is. Note that this will be stored in a number. In 
 *   the top of ClassGate.h, you will find a list of #define macros that give names to each gate type.
 *   For example, we will use value 0 to represent a NAND gate. However, instead of remembering that, you
 *   can simply use the macros like: "if (gateType == GATE_NAND)". If you want to get the type of a gate
 *   object, call the get_gateType() function.
 *   
 *   One important thing to note: if this gate represents a primary input (PI) of the circuit, then
 *   the gate will be of type GATE_PI.
 * 
 * - Vectors of pointers to this gate's predecessors and successors. The predecessors are the gates 
 *   whose outputs are the inputs to this gate. (If this gate is a PI, then it has no predecessors.)
 *   The successors are the gates who take in the input of this gate. (If this gate drives a PO, then
 *   it has no successors. You can access these vectors by running the \a get_gateInputs() and
 *   \a get_gateOutputs() functions.
 * 
 * - A logical value that indicates the output value of the gate in the current simulation. Currently,
 *   legal values are, 0, 1, X, and "unset." The "unset" value indicates that the value is unknown because
 *   it has not yet been computed. The X value indicates a true "unknown" input value. You can get the
 *   value of a gate's output by using the \a getValue() function, and set it using the \a setValue() 
 *   function. 
 *   
 *   These values are encoded as numbers, but (like the gate type) you will use macros defined at the
 *   top of ClassGate.h (instead of hard-coding the numbers into the system). For example, to set the
 *   value of a gate to 1, you would call setValue(LOGIC_ONE).
 *
 *   Later we will be adding other values (such "should be zero in a fault-free circuit, but is one due
 *   to a fault.")
 * 
 * - A detectable Fault list, stored as a vector<Fault>. You will use this list in Project 2 to store
 *   the detectable faults on each signal that you calculate using deductive fault simulation.
 */
 
#include "ClassGate.h"

/** \brief Constructor for a new Gate.
 *  \param name a string providing the output name for the gate
 *  \param ID a unique ID number for the gate (primarily used while parsing the input source file)
 *  \param gt the type of gate, using the GATE_* marcos defined in ClassGate.h
 */
Gate::Gate(string name, int ID, int gt) {
  outputName = name; 
  gateID = ID; 
  gateType = gt; 
  gateValue = LOGIC_UNSET;
}
  
/** \brief Get the gate type for this gate.
 *  \return The gate type, using the GATE_* macros defined in ClassGate.h
 */
char Gate::get_gateType() { return gateType; }

/** \brief Get the gate's output pointers.
 *  \return A vector of pointers to the gates that this gate's output connects to.
 */
vector<Gate*> Gate::get_gateOutputs() { return gateOutputs; }

/** \brief Add a pointer to Gate \a x as an output destination of this gate.
 *  \param x A pointer to a Gate that takes this Gate's output as input.
 *  \note This code should only need to be run by the setupCircuit() function of Circut. You should never have to run this.
 */
void Gate::set_gateOutput(Gate* x) { gateOutputs.push_back(x); }


/** \brief In this Gate's output list, replace the pointer to \a oldGate with a pointer to \a newGate.
 *  \param oldGate Old gate to remove
 *  \param newGate New pointer
 *  \note This code should only need to be run by the setupCircuit() function of Circut. You should never have to run this.
 */
void Gate::replace_gateOutput(Gate* oldGate, Gate* newGate) { 
  for (int i=0; i < gateOutputs.size(); i++) {
    if (gateOutputs[i] == oldGate) {
      gateOutputs[i] = newGate;
      return;
    }
  }
	
  assert(false);
  return;
}

/** \brief Get the gate's input pointers.
 *  \return A vector of pointers to the gates whose outputs connect to this gate's inputs.
 */
vector<Gate*> Gate::get_gateInputs() { return gateInputs; }

/** \brief Add a pointer to Gate \a x as an input source of this gate.
 *  \param x A pointer to a Gate whose output is an input to this gate.
 *  \note This code should only need to be run by the setupCircuit() function of Circut. You should never have to run this.
 */
void Gate::set_gateInput(Gate* x) { 
  gateInputs.push_back(x); 
}	

/** \brief In this Gate's input list, replace the pointer to \a oldGate with a pointer to \a newGate.
 *  \param oldGate Old gate to remove
 *  \param newGate New pointer
 *  \note This code should only need to be run by the setupCircuit() function of Circut. You should never have to run this.
 */
void Gate::replace_gateInput(Gate* oldGate, Gate* newGate) {
  for (int i=0; i < gateInputs.size(); i++) {
    if (gateInputs[i] == oldGate) {
      gateInputs[i] = newGate;
      return;
    }
  }
	
  assert(false);
  return;
}

/** \brief Get the name of output of this gate.
 * \return A string containing the name of the output of this gate.
 */
string Gate::get_outputName() { return outputName; }			


/** \brief Print information about this gate.
 */			
void Gate::printGateInfo() {
  cout << "Gate " << gateID << ": " << outputName << " = " << gateTypeName() << "(";
  
  if (gateInputs.size() > 0) {    
    for (int j=0; j < gateInputs.size()-1; j++) {
      cout << gateInputs[j]->get_outputName();    
      cout << ", ";
    }
    cout << gateInputs[gateInputs.size()-1]->get_outputName();
  }

  cout << ")";
  if (gateValue != LOGIC_UNSET)
    cout << " = " << printLogicVal(gateValue) << ";";
  else
    cout << ";";
    
  if (detectableFaults.size() > 0) {
    cout << " {";
    for (int i=0; i<detectableFaults.size()-1; i++) 
      cout << printFault(detectableFaults[i]) << ", ";
    cout << printFault(detectableFaults[detectableFaults.size()-1]) << "}";
  }


  cout << endl;  		
}

/** \brief Convert a logic value (e.g. LOGIC_ZERO) to a printable string (e.g. "0").
 *  \param val The logic value (based on the macros in ClassGate.h)
 */
string Gate::printLogicVal(int val) {
  switch(val) {
  case LOGIC_ZERO: return "0";
  case LOGIC_ONE: return "1";
  case LOGIC_UNSET: return "U";
  case LOGIC_X: return "X";
  }

  cout << "ERROR: Do not know how to print logic value " << val << " in Gate::printLogicVal(int val)" << endl;
  return "";
}

/** \brief Get this gate's type as a string (e.g., GATE_NAND --> "NAND").
 *  \return A string with the name of this gate's type.
 */
string Gate::gateTypeName() {
  // macros defined in ClassGate.h
  switch(gateType) {   
  case GATE_NAND: return "NAND";
  case GATE_NOR: return "NOR";
  case GATE_AND: return "AND";
  case GATE_OR: return "OR";
  case GATE_XOR: return "XOR";
  case GATE_XNOR: return "XNOR";
  case GATE_BUFF: return "BUFF";
  case GATE_NOT: return "NOT";
  case GATE_PI: return "PI";
  case GATE_FANOUT: return "FANOUT";
  default: return "ERROR";
  }
}

/** \brief Sets the value of this gate.
 *  \param val The logic value (based on the LOGIC_* marcos in ClassGate.h).
 */
void Gate::setValue(int val) { 
  gateValue = val; 
}

/** \brief Gets the value of this gate's output.
 * 
 *  \return The logic value (based on the LOGIC_* marcos in ClassGate.h).
 */
char Gate::getValue() { 
  return gateValue;
}

/** \brief Gets the value of this's gate's output as a string (intended for printing).
 *  Instead of returning LOGIC_ZERO, etc., it will just return a printable version (e.g. 0)
 *  \return The logical value of this gate as a string.
 */
string Gate::printValue() {
  return printLogicVal(gateValue);
}


/** \brief Stores the name of one of this gate's input signals.
 *  \param n The name of one of this gate's inputs.
 *  \note Normally, this code should only need to be run by the parser, and its results should only need to be used by the setupCircuit() function of Circut. You should never need to touch this.
 */
void Gate::set_gateInputName(string n) {
  inputName.push_back(n);
}

/** \brief Gets the pre-stored names of this gate's input signals. 
 *  Normally, this code should only need to be run by the setupCircuit() function of Circut.
 */
vector<string> Gate::get_gateInputNames() {
  return inputName;
}

/** \brief Finds which of this gate's inputs is connected to the output of Gate \a g.
 *  \param g Pointer to the \a Gate we are searching for
 *  \return An integer value, showing which of this gate's inputs (numbered starting at 0) is connect to \a Gate \a g.
 * 
 *  A \a Gate \a t contains \a gateInputs, a vector of pointers to the gates which drive the inputs of \a t. 
 *  This function will tell us which location in \a gateInputs holds \a *g.
 *  If \a *g is not found, then this function prints a warning and returns -1.
 */
int Gate::getGateInputNumber(Gate *g) {
  for (int i=0; i<gateInputs.size(); i++) {
    if (gateInputs[i] == g)
      return i;
  }

  cout << "Warning: requested gate not found in Gate.getGateInputNumber" << endl;
  return -1;
}

/** \brief Adds a Fault \a f to this Gate's detectable fault list.
 *  \param f The Fault to be added.
 */
void Gate::add_detectableFault(Fault f) {
  detectableFaults.push_back(f);
}

/** \brief Returns this Gate's detectable fault list.
 */
vector<Fault> Gate::get_detectableFaults() {return detectableFaults; }

/** \brief Prints the detectable faults on this Gate. */
void Gate::print_detectableFaults() {
  for (int i=0; i<detectableFaults.size(); i++) {
    cout << printFault(detectableFaults[i]) << endl;
  }
}

/** \brief Clear's this gate's detectable fault list. */
void Gate::clear_detectableFaults() {
  detectableFaults.clear();
}

/** \brief Converts a Fault into a printable string.
 *  \param f The Fault to be converted to a string.
 */
string Gate::printFault(Fault f) {
  ostringstream ss;
  ss << f.loc->get_outputName() << "/" << (int)(f.type);
  return ss.str();
}

/** \brief Sets this Gate's detectable fault list to the given vector<Fault>.
 *  \param f The detectable fault list to set this Gate's to.
 */
void Gate::set_detectableFaults(vector<Fault> f) { detectableFaults = f; }
