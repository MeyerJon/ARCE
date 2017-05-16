#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <map>
#include <algorithm>

#include "../FA/FA.h"
#include "../FA/State.h"
#include "../FA/DFA.h"
#include "../FA/eNFA.h"



using namespace std;

void printSS(std::vector<State> ss){
    std::string out = "{";
    for(State& s : ss){
        out += s.getName() + ", ";
    }
    out.pop_back(); out.pop_back();
    out += "}";
    std::cout << out;
}

std::string getSSname(std::vector<State> ss){
    std::string name = "{";
    for(State s : ss){
        name += s.getName() + ", ";
    }
    name.pop_back(); name.pop_back(); // Get rid of the last ', '
    name += "}";
    return name;
}

DFA MSSC(eNFA& enfa){

    typedef std::vector<State> subset;
    typedef std::tuple<std::string, std::string, std::string> transInfo;
    // Lazy eval
    std::set<std::string> alph = enfa.getAlphabet();
    alph.erase(std::remove(alph.begin(), alph.end(), enfa.getEpsilon()), alph.end());
    map<tuple<State*, string>, set<State*>> transtable = enfa.getTransitions();
    std::vector<transInfo> transitions;
    std::vector<State*> states;
    std::vector<subset> allSubsets;
    std::vector<std::string> allSubsetNames; // For comparing purposes
    bool finished = false;

    // Find the starting subset/state
    set<State*> startSSset = enfa.eclose(enfa.getStartstate());
    subset startSS;
    std::copy(startSSset.begin(), startSSset.end(), std::back_inserter(startSS));
    allSubsets.push_back(startSS);
    allSubsetNames.push_back(getSSname(startSS));

    // Complete the algorithm
    // Go over all newfound subsets
while(!finished) {
    finished = true;
    for (subset ss : allSubsets) {
        // Go over every character in the alphabet
        for (std::string c : alph) {
            // Go over every state in the ss
            subset newSubset = {};
            for (State* s : ss) {
                // Collect all states accessible from this state with c
                std::pair<State*, std::string> arg = std::make_pair(s, c);

                // Take the eclose of this ss
                if (transtable.find(arg) == transtable.end()) continue;
                std::set<State*> ecl = enfa.ecloseSubset(transtable[arg]);
                std::copy(ecl.begin(), ecl.end(), std::back_inserter(newSubset));
                std::set<State*> noDupes(newSubset.begin(), newSubset.end());
                newSubset.assign(noDupes.begin(), noDupes.end()); // Remove duplicates

                // Check if this subset is new (it's not found in 'allSubsets')
                if (find(allSubsetNames.begin(), allSubsetNames.end(), getSSname(newSubset)) == allSubsetNames.end()) {
                    // If so, add to found subsets
                    allSubsetNames.push_back(getSSname(newSubset));
                    allSubsets.push_back(newSubset);
                    finished = false;
                    // Save the transition info
                    transInfo trans = {getSSname(ss), getSSname(newSubset), c};
                    transitions.push_back(trans);
                }
                else if(find(allSubsetNames.begin(), allSubsetNames.end(), getSSname(newSubset)) != allSubsetNames.end()){
                    // The subset is already in there, but a transition might be needed still
                    transInfo trans = {getSSname(ss), getSSname(newSubset), c};
                    transitions.push_back(trans);
                }
            }
        }
    }
}
    // Create states out of all subsets
    bool startingFound = false;
    for(subset ss : allSubsets){
        std::string name = getSSname(ss);
        bool starts = false;
        bool accepts = false;
        for(State* s : ss){
            if(s->isStarting() && !startingFound){
                starts = true; startingFound = true;
            }
            if(s->isAccepting()) accepts = true;
        }
        State* state = new State(name, starts, accepts);
        states.push_back(state);
    }

    // Create DFA with states and alphabet
    State* deadState = new State("Garbage", false, false); // Garbage state for later
    states.push_back(deadState);

    DFA dfa = DFA();
    dfa.setAlphabet(alph);

    // Add transitions
    for(transInfo trans : transitions){
        std::string fromName = std::get<0>(trans);
        std::string toName = std::get<1>(trans);
        State* from = nullptr;
        State* to = nullptr;
        // TODO: temp fix
        for(State* s : states){
            if(s->getName() == fromName) from = s;
            if(s->getName() == toName) to = s;
            if(to != nullptr and from != nullptr) break;
        }
        std::string c = std::get<2>(trans);
        dfa.addTransition(from, c ,to);
    }

    // Taking care of a garbage state for transitions that "should exist, but don't"
    map<tuple<State*, string>, set<State*>> dfaTransitions = dfa.getTransitions();
    for(State* s : dfa.getStates()){
        for(std::string c : alph){
            std::pair<State, std::string> arg = std::make_pair(s, c);
            // If no entry exists for this char
            if(dfaTransitions.find(arg) == dfaTransitions.end()){
                //std::cout << "No transition from " << s.getName() << " with " << c << ".\n";
                // Point it to the garbage state
                dfa.addTransition(s, c, deadState);
            }
        }
    }

    return dfa;

}