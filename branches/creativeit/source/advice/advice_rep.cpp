#include "advice_rep.h"
#include "rtneat/nnode.h"
#include "rtneat/gene.h"
#include <iostream>
#include <climits>

using namespace NEAT;
using namespace std;

// Static members and extern variables.
const F64 Eterm::mOmega;
const F64 SetVar::mOmega;
const F64 BooleanTerm::mOmega;
const F64 BinaryTerm::mOmega;
const F64 Conds::mOmega;
const F64 IfRule::mOmega;
U32 Variable::mNumSensors;
U32 Variable::mNumActions;
U32 Counters::mLine;
Agent::Type Agent::mType;


void BooleanTerm::buildRepresentation(PopulationPtr population, NNodePtr biasnode, GenomePtr genome, std::vector<NNodePtr>& variables, NNodePtr& ionode) const {
    // Create a hidden node to compute the output of the term.
    NNodePtr newnode(new NNode(NEURON, population->cur_node_id++, HIDDEN));
    genome->nodes.push_back(newnode);
    genome->genes.push_back(GenePtr(new Gene(mWeight, biasnode, newnode, false, 0, mWeight)));

    ionode = newnode;
}

void BinaryTerm::buildRepresentation(PopulationPtr population, NNodePtr biasnode, GenomePtr genome, std::vector<NNodePtr>& variables, NNodePtr& ionode) const {
    // Create a hidden node to compute the output of the term.
    NNodePtr newnode(new NNode(NEURON, population->cur_node_id++, HIDDEN));
    genome->nodes.push_back(newnode);

    // The bias node is one input, and mVariable and mRHSVar denote the other inputs.
    NNodePtr varnode = Variable::getNode(population, genome, variables, mVariable);
    genome->genes.push_back(GenePtr(new Gene(mWeight1, varnode, newnode, false, 0, mWeight1)));
    if (mRHSType == eVariable && mWeight2 != 0.0) {
        NNodePtr rhsnode = Variable::getNode(population, genome, variables, mRHSVar);
        genome->genes.push_back(GenePtr(new Gene(mWeight2, rhsnode, newnode, false, 0, mWeight2)));
    }
    if (mWeight3 != 0.0) {
        genome->genes.push_back(GenePtr(new Gene(mWeight3, biasnode, newnode, false, 0, mWeight3)));
    }

    ionode = newnode;
}

void Conds::buildRepresentation(PopulationPtr population, NNodePtr biasnode, GenomePtr genome, std::vector<NNodePtr>& variables, NNodePtr& ionode) const {
    // Consider ionode, which contains the truth value of previous conditions in nested
    // if-then-else statements, as an additional term in the condition if it is not biasnode.
    std::vector<NNodePtr> termnodes;
    F64 weight2;
    if (ionode == biasnode) {
        weight2 = mWeight2;
    }
    else {
        termnodes.push_back(ionode);
        weight2 = mOmega*(-2.0*(mTerms.size()+1)+1.0)/2.0;
    }

    // Build representations for each term in this condition.
    for (U32 i = 0; i < mTerms.size(); i++) {
        mTerms[i]->buildRepresentation(population, biasnode, genome, variables, ionode);
        termnodes.push_back(ionode);
    }

    if (termnodes.size() > 1) {
        // Create a hidden node to compute the combined output of the terms.
        NNodePtr newnode(new NNode(NEURON, population->cur_node_id++, HIDDEN));
        genome->nodes.push_back(newnode);

        // The condition is the conjunction of the constituent terms.
        // Make connections to the hidden node from each term and from the bias node.
        for (U32 i = 0; i < termnodes.size(); i++) {
            genome->genes.push_back(GenePtr(new Gene(mWeight1, termnodes[i], newnode, false, 0, mWeight1)));
        }
        genome->genes.push_back(GenePtr(new Gene(weight2, biasnode, newnode, false, 0, weight2)));

        ionode = newnode;
    }
}

void IfRule::buildRepresentation(PopulationPtr population, NNodePtr biasnode, GenomePtr genome, std::vector<NNodePtr>& variables, NNodePtr& ionode) const {
    // Save the ionode since it contains value of previous condition that we will need
    // to compute the else node.
    NNodePtr prvionode = ionode;

    // The output of the condition is the then node, which is connected to the then rule.
    mConds->buildRepresentation(population, biasnode, genome, variables, ionode);
    NNodePtr thennode = ionode;
    mThen->buildRepresentation(population, biasnode, genome, variables, ionode);

    if (mElse != NULL) {
        // The else node is obtained by negating the then node.  In general, we think of
        // the else node as the conjunction of previous condition (positive antecedent) and
        // negation of the then node (negative antecedent).
        NNodePtr elsenode(new NNode(NEURON, population->cur_node_id++, HIDDEN));
        genome->nodes.push_back(elsenode);
        genome->genes.push_back(GenePtr(new Gene(-mWeight1, thennode, elsenode, false, 0, -mWeight1)));
        if (prvionode == biasnode) {
            genome->genes.push_back(GenePtr(new Gene(mWeight2, biasnode, elsenode, false, 0, mWeight2)));
        }
        else {
            genome->genes.push_back(GenePtr(new Gene(mWeight1, prvionode, elsenode, false, 0, mWeight1)));
            genome->genes.push_back(GenePtr(new Gene(-mWeight2, biasnode, elsenode, false, 0, -mWeight2)));
        }

        // The else node is connected to the else rule.
        mElse->buildRepresentation(population, biasnode, genome, variables, elsenode);
    }
}

void SetRules::buildRepresentation(PopulationPtr population, NNodePtr biasnode, GenomePtr genome, std::vector<NNodePtr>& variables, NNodePtr& ionode) const {
    // Build representations for each assignment.
    for (U32 i = 0; i < mSetVars.size(); i++) {
        NNodePtr tmpnode = ionode;  // buildRepresentation() below may change tmpnode, so make copy
        mSetVars[i]->buildRepresentation(population, biasnode, genome, variables, tmpnode);
    }
}

void Eterm::buildRepresentation(PopulationPtr population, NNodePtr biasnode, GenomePtr genome, std::vector<NNodePtr>& variables, NNodePtr& ionode) const {
    // Create a hidden node to compute the output.
    NNodePtr newnode(new NNode(NEURON, population->cur_node_id++, HIDDEN));
    genome->nodes.push_back(newnode);

    // ionode contains the if/else node, so connect the new node from it.
    genome->genes.push_back(GenePtr(new Gene(mWeight3, ionode, newnode, false, 0, mWeight3)));
    genome->genes.push_back(GenePtr(new Gene(mWeight2, biasnode, newnode, false, 0, mWeight2)));
    if (mType == eVariable) {
        NNodePtr varnode = Variable::getNode(population, genome, variables, mVariable);
        genome->genes.push_back(GenePtr(new Gene(mWeight1, varnode, newnode, false, 0, mWeight1)));
    }

    ionode = newnode;
}

void SetVar::buildRepresentation(PopulationPtr population, NNodePtr biasnode, GenomePtr genome, std::vector<NNodePtr>& variables, NNodePtr& ionode) const {
    // Only accumulation is supported for variables in the network.
    if (mType == eAssignment) {
        throw std::runtime_error("Assignment to network variables is not supported; use accumulation");
    }

    // Build representations for each eterm, and connect their outputs to varnode.
    NNodePtr varnode = Variable::getNode(population, genome, variables, mVariable);
    const std::vector<Eterm*>& eterms = mExpr->getEterms();
    for (U32 i = 0; i < eterms.size(); i++) {
        NNodePtr tmpnode = ionode;  // buildRepresentation() below may change tmpnode, so make copy
        eterms[i]->buildRepresentation(population, biasnode, genome, variables, tmpnode);

        // Now link the new node (tmpnode) to varnode with y = sigmoid(4*x - 2) connection
        // (also scale the constants appropriately).  However, instead of using the bias node
        // to form the constant 2, use the condition node (ionode). When condition is false,
        // activation of tmpnode ~ 0 and value of the constant ~ 0, so that contribution to
        // varnode ~ 0.  When condition is true, constant ~ 2 and the linear region of
        // y = sigmoid(4*x - 2) applies for the contribution to varnode.
        F64 scale = eterms[i]->getScale();
        genome->genes.push_back(GenePtr(new Gene(scale*mWeight1, tmpnode, varnode, false, 0, scale*mWeight1)));
        genome->genes.push_back(GenePtr(new Gene(scale*mWeight2, ionode, varnode, false, 0, scale*mWeight2)));
    }
}

void Rules::buildRepresentation(PopulationPtr population, NNodePtr biasnode, GenomePtr genome, std::vector<NNodePtr>& variables, NNodePtr& ionode) const {
    for (U32 i = 0; i < mRules.size(); i++) {
        NNodePtr tmpnode = ionode;  // buildRepresentation() below may change tmpnode, so make copy
        mRules[i]->buildRepresentation(population, biasnode, genome, variables, tmpnode);
    }
}

bool BinaryTerm::evaluate(std::vector<F64>& variables) const {
    if (mRHSType == eValue) {
        return compare(Variable::getValue(variables, mVariable), mRHSVal);
    }
    else if (mRHSType == eVariable) {
        return compare(Variable::getValue(variables, mVariable), Variable::getValue(variables, mRHSVar));
    }
}

bool Conds::evaluate(std::vector<F64>& variables) const {
    // The result is the conjunction of the individual terms.
    bool result = true;
    for (U32 i = 0; i < mTerms.size() && result; i++) {
        result = result && mTerms[i]->evaluate(variables);
    }
    return result;
}

void IfRule::evaluate(std::vector<F64>& variables) const {
    if (mConds->evaluate(variables)) {
        mThen->evaluate(variables);
    }
    else if (mElse != NULL) {
        mElse->evaluate(variables);
    }
}

void SetRules::evaluate(std::vector<F64>& variables) const {
    for (U32 i = 0; i < mSetVars.size(); i++) {
        mSetVars[i]->evaluate(variables);
    }
}

F64 Eterm::evaluate(std::vector<F64>& variables) const {
    if (mType == eValue) {
        return mValue;
    }
    else if (mType == eVariable) {
        return mValue*Variable::getValue(variables, mVariable);
    }
    else {
        return numeric_limits<F64>::quiet_NaN();
    }
}

F64 Expr::evaluate(std::vector<F64>& variables) const {
    F64 sum = 0.0;
    for (U32 i = 0; i < mEterms.size(); i++) {
        sum += mEterms[i]->evaluate(variables);
    }
    return sum;
}

void SetVar::evaluate(std::vector<F64>& variables) const {
    if (mType == eAssignment) {
        Variable::setValue(variables, mVariable, mExpr->evaluate(variables));
    }
    else {
        F64 curValue = Variable::getValue(variables, mVariable);
        Variable::setValue(variables, mVariable, curValue + mExpr->evaluate(variables));
    }
}

void Rules::evaluate(std::vector<F64>& variables) const {
    for (U32 i = 0; i < mRules.size(); i++) {
        mRules[i]->evaluate(variables);
    }
}


void LEQTerm::print() const {
    if (mRHSType == eValue) printf("%s <= %f ", Variable::toString(mVariable).c_str(), Number::unscale(mRHSVal, mVariable));
    else if (mRHSType == eVariable) printf("%s <= %s ", Variable::toString(mVariable).c_str(), Variable::toString(mRHSVar).c_str());
}

void LTTerm::print() const {
    if (mRHSType == eValue) printf("%s < %f ", Variable::toString(mVariable).c_str(), Number::unscale(mRHSVal, mVariable));
    else if (mRHSType == eVariable) printf("%s < %s ", Variable::toString(mVariable).c_str(), Variable::toString(mRHSVar).c_str());
}

void GEQTerm::print() const {
    if (mRHSType == eValue) printf("%s >= %f ", Variable::toString(mVariable).c_str(), Number::unscale(mRHSVal, mVariable));
    else if (mRHSType == eVariable) printf("%s >= %s ", Variable::toString(mVariable).c_str(), Variable::toString(mRHSVar).c_str());
}

void GTTerm::print() const {
    if (mRHSType == eValue) printf("%s > %f ", Variable::toString(mVariable).c_str(), Number::unscale(mRHSVal, mVariable));
    else if (mRHSType == eVariable) printf("%s > %s ", Variable::toString(mVariable).c_str(), Variable::toString(mRHSVar).c_str());
}

void IfRule::print() const {
    printf("if ");
    mConds->print();
    printf("then ");
    mThen->print();
    if (mElse != NULL) {
        printf("else ");
        mElse->print();
    }
    printf("endif ");
}

void Conds::print() const {
    for (vector<Term*>::const_iterator iter = mTerms.begin(); iter != mTerms.end(); iter++) {
        (*iter)->print();
        if (iter + 1 != mTerms.end()) {
            printf("and ");
        }
    }
}

void SetRules::print() const {
    printf("{ ");
    for (vector<SetVar*>::const_iterator iter = mSetVars.begin(); iter != mSetVars.end(); iter++) {
        (*iter)->print();
    }
    printf("} ");
}

void Eterm::print() const {
    if (mType == eValue) {
        // Terms are in the right-hand side of assignment/accumulation expressions.
        // Since the variable on the left-hand side of such expressions is guaranteed
        // to be a non-sensor variable, we can unscale the number without checking the
        // variable type.
        printf("%f ", Number::unscale(mValue));
    }
    else if (mType == eVariable) {
        if (mValue != 1.0) printf("%f*", mValue);
        printf("%s ", Variable::toString(mVariable).c_str());
    }
}

void Expr::print() const {
    for (vector<Eterm*>::const_iterator iter = mEterms.begin(); iter != mEterms.end(); iter++) {
        (*iter)->print();
        if (iter + 1 != mEterms.end()) {
            printf("+ ");
        }
    }
}

void SetVar::print() const {
    printf("%s ", Variable::toString(mVariable).c_str());
    if (mType == eAssignment) printf("= ");
    else printf("+= ");
    mExpr->print();
}


void Rules::print() const {
    for (U32 i = 0; i < mRules.size(); i++) {
        mRules[i]->print();
        printf("\n");
    }
}


// Translate variable of a given type and index into an unsigned integer.
U32 Variable::translate(Type type, U32 index) {
    switch (type) {
    case eSensor:
        if (index < mNumSensors) {
            return index;
        }
        else {
            translateError(type, index, "Sensor variable out of range");
        }
        break;
    case eEvolvedAction:
        if (index < mNumActions) {
            if (Agent::mType == Agent::eEvolved) {
                return mNumSensors+index;
            }
            else {
                translateError(type, index, "Invalid action variable for scripted agent");
            }
        }
        else {
            translateError(type, index, "Action variable out of range");
        }
        break;
    case eAction:
        if (index < mNumActions) {
            if (Agent::mType == Agent::eEvolved) {
                return mNumSensors+mNumActions+index;
            }
            else {
                return mNumSensors+index;
            }
        }
        else {
            translateError(type, index, "Action variable out of range");
        }
        break;
    case eGeneral:
        if (Agent::mType == Agent::eEvolved && mNumSensors+2*mNumActions+index < UINT_MAX) {
            return mNumSensors+2*mNumActions+index;
        }
        else if (mNumSensors+mNumActions+index < UINT_MAX) {
            return mNumSensors+mNumActions+index;
        }
        else {
            translateError(type, index, "General purpose variable out of range");
        }
        break;
    default:
        translateError(type, index, "Unknown variable type");
        break;
    }
}

// Function to handle error while translating variables.
void Variable::translateError(Type type, U32 index, std::string message) {
    printf("\n");
    std::ostringstream oss;
    oss << message << ": " << toString(type, index) << " at line number " << Counters::mLine;
    throw std::runtime_error(oss.str());
}

// Given a translated variable, return its type.
Variable::Type Variable::getType(U32 var) {
    if (var < mNumSensors) {
        return eSensor;
    }
    else if (var < mNumSensors+mNumActions) {
        if (Agent::mType == Agent::eEvolved) return eEvolvedAction;
        else return eAction;
    }
    else if (var < mNumSensors+2*mNumActions) {
        if (Agent::mType == Agent::eEvolved) return eAction;
        else return eGeneral;
    }
    else {
        return eGeneral;
    }
}

// Given a translated variable, return its index.
U32 Variable::getIndex(U32 var) {
    if (var < mNumSensors) {
        return var;
    }
    else if (var < mNumSensors+mNumActions) {
        return var - mNumSensors;
    }
    else if (var < mNumSensors+2*mNumActions) {
        return var - (mNumSensors+mNumActions);
    }
    else {
        if (Agent::mType == Agent::eEvolved) return var - (mNumSensors+2*mNumActions);
        else return var - (mNumSensors+mNumActions);
    }
}

// Convert a translated variable to string suitable for printing.
std::string Variable::toString(U32 var) {
    return toString(getType(var), getIndex(var));
}
    
// Convert the given type and index to string suitable for printing.
std::string Variable::toString(Type type, U32 index) {
    std::ostringstream oss;
    switch (type) {
    case eSensor:
        oss << "s" << index;
        break;
    case eEvolvedAction:
        oss << "ea" << index;
        break;
    case eAction:
        oss << "a" << index;
        break;
    case eGeneral:
        oss << "g" << index;
        break;
    default:
        break;
    }
    return oss.str();
}
    
// Get the neural network node corresponding to the given translated variable.
NNodePtr Variable::getNode(PopulationPtr population, GenomePtr genome, std::vector<NNodePtr>& variables, U32 var) {
    // If the node for the given var doesn't exist, create a hidden node for that var.
    if (var >= variables.size()) variables.resize(var+1, NNodePtr());
    if (variables[var] == NNodePtr()) {
        NNodePtr newnode(new NNode(NEURON, population->cur_node_id++, HIDDEN));
        genome->nodes.push_back(newnode);
        variables[var] = newnode;
    }

    return variables[var];
}

// Get the value of the given translated variable.
F64 Variable::getValue(std::vector<F64>& variables, U32 var) {
    if (var >= variables.size()) variables.resize(var+1, 0.0);
    return variables[var];
}

// Set the value of the given translated variable.
void Variable::setValue(std::vector<F64>& variables, U32 var, F64 val) {
    if (var >= variables.size()) variables.resize(var+1, 0.0);
    variables[var] = val;
}


// Scale a value in [-1, 1] to [0.2, 0.8].
F64 Number::scale(F64 val) {
    if (Agent::mType == Agent::eEvolved) {
        return 0.8 - (1.0-val)*0.6/2.0;
    }
    else {
        return val;
    }
}

// Scale a value in [-1, 1] to [0.2, 0.8] if var is non-sensor.
F64 Number::scale(F64 val, U32 var) {
    if (Variable::getType(var) == Variable::eSensor) {
        return val;
    }
    else {
        return Number::scale(val);
    }
}

// This is the inverse operation of the above scaling.
F64 Number::unscale(F64 val) {
    if (Agent::mType == Agent::eEvolved) {
        return 1.0 - (0.8-val)*2.0/0.6;
    }
    else {
        return val;
    }
}

F64 Number::unscale(F64 val, U32 var) {
    if (Variable::getType(var) == Variable::eSensor) {
        return val;
    }
    else {
        return Number::unscale(val);
    }
}

// Make sure the value is in the range [-1, 1].
F64 Number::checkRange(F64 val) {
    if (val >= -1 && val <= 1) {
        return val;
    }
    else {
        printf("\n");
        std::ostringstream oss;
        oss << "Number out of range: " << val << " at line number " << Counters::mLine;
        throw std::runtime_error(oss.str());
    }
}