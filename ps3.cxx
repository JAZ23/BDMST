//	Author: Christopher Lee Jackson & Jason Jones
//	Course: CMPSC463
//  Problem: 3-1
//  Description:


#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <algorithm>
#include <vector>
#include "Graph.h"
#include "BinaryHeap.h"
#include <cmath>
#include <cstring>

using namespace std;

typedef struct {
	int data; // initial vertex ant started on
	Vertex *location;
	vector<int> *visited;
}Ant;

typedef struct {
	double low;
	double high;
	Edge *assocEdge;
}Range;

//	Globals
const double P_UPDATE_EVAP = 0.95;
const double P_UPDATE_ENHA = 1.05;
const int TABU_MODIFIER = 5;
const int MAX_CYCLES = 2500; // change back to 2500

double loopCount = 0;
double evap_factor = 0.5;
double enha_factor = 1.5;
double maxCost = 0;
double minCost = std::numeric_limits<double>::infinity();

int cycles = 1;
int totalCycles = 1;

//	Prototypes
void processFile(Graph *g, char* file);
void processFileOld(Graph *g, char* file);
void processFileNew(Graph *g, char* file);
vector<Edge*> AB_DBMST(Graph *g, int d);
vector<Edge*> treeConstruct(Graph *g, int d);
bool asc_cmp_plevel(Edge *a, Edge *b);
bool des_cmp_cost(Edge *a, Edge *b);
bool asc_src(Edge *a, Edge *b);
void move(Graph *g, Ant *a);
void updatePheromonesPerEdge(Graph *g);
void updatePheromonesGlobal(Graph *g, vector<Edge*> best, bool improved);
void printEdge(Edge* e);
int findRoot(Vertex* v, vector<int> uf);
bool looping(Edge* e, vector<int> uf);

int main( int argc, char *argv[])
{
    //  Process input from command line
   // if (argc != 3) {
  //      cerr << "Wrong input.\n";
   // }
    char* fileName = new char[50];
    strcpy(fileName,argv[1]);
    int d;
    d = atoi(argv[2]);
    //  Process input file and get resulting graph
	Graph *g = new Graph();
    //processFile(g, fileName);
    processFileNew(g, fileName);
	//g->print();
	cout << "Diameter Bound\n";
	vector<Edge*> best = AB_DBMST(g, d);
	sort(best.begin(), best.end(), asc_src);
    cout << "Best Tree num edges: " << best.size() << endl;
	for_each(best.begin(), best.end(), printEdge);
	return 0;
}

/*
*
*
*	For Each, Sort - Helper Functions
*
*/
bool asc_cmp_plevel(Edge *a, Edge *b) {
	return (a->pLevel < b->pLevel);
}

bool des_cmp_cost(Edge *a, Edge *b) {
	return (a->weight > b->weight);
}

bool asc_src(Edge* a, Edge* b) {
	return (a->getSource(NULL)->data < b->getSource(NULL)->data);
}

void printEdge(Edge* e) {
	cout << e->getSource(NULL)->data << " " << e->getDestination(NULL)->data << " " << e->weight << " " << e->pLevel << endl;
}


vector<Edge*> AB_DBMST(Graph *g, int d) {
	//	Local Declerations
	double bestCost = std::numeric_limits<double>::infinity();
	double treeCost = 0;
	const int s = 75;
	vector<Edge*> best, current;
	Vertex *vertWalkPtr;
	Edge *edgeWalkPtr;
	vector<Ant*> ants;
	vector<Edge*>::iterator e, ed;
	Ant *a;
	//	Assign one ant to each vertex
	vertWalkPtr = g->getFirst();
	for (unsigned int i = 0; i < g->getCount(); i++) {
		Ant *a = new Ant;
		a->data = i +1;
		a->location = vertWalkPtr;
		a->visited = new vector<int>(g->getCount(), 0);
		ants.push_back(a);
		//	Initialize pheremone level of each edge, and set pUdatesNeeded to zero
		for ( e = vertWalkPtr->edges.begin() ; e < vertWalkPtr->edges.end(); e++ ) {
			edgeWalkPtr = *e;
			if (edgeWalkPtr->getSource(NULL) == vertWalkPtr) {
				edgeWalkPtr->pUpdatesNeeded = 0;
				edgeWalkPtr->pLevel = (maxCost - edgeWalkPtr->weight) + ((maxCost - minCost) / 3);
			}
		}
		//	Done with this vertex's edges; move on to next vertex
		vertWalkPtr = vertWalkPtr->pNextVert;
	}
	while (totalCycles <= 10000 && cycles <= MAX_CYCLES) { 
		cerr << "Cycle" << totalCycles << endl;
		if(totalCycles % 100 == 0) 
			cerr << "CYCLE " << totalCycles << endl;
		//	Exploration Stage
		for (int step = 1; step <= s; step++) {
			if (step == s/3 || step == (2*s)/3) {
				updatePheromonesPerEdge(g);
			}
			for (unsigned int j = 0; j < g->getCount(); j++) {
				a = ants[j];
				move(g, a);
			}
            if ( step % TABU_MODIFIER == 0 ) {
                for(unsigned int w = 0; w < g->getCount(); w++) {
        			ants[w]->visited->assign(g->getCount(), 0); //  RESET VISITED FOR EACH ANT (TABU)
        		}
            }
		}
		for(unsigned int w = 0; w < g->getCount(); w++) {
			ants[w]->visited->assign(g->getCount(), 0); //  RESET VISITED FOR EACH ANT
		}
		updatePheromonesPerEdge(g);
		//	Tree Construction Stage
		current = treeConstruct(g, d);
		//	Get new tree cost
		for ( ed = current.begin() ; ed < current.end(); ed++ ) {
			edgeWalkPtr = *ed;
			treeCost+=edgeWalkPtr->weight;
		}
		if (treeCost < bestCost) {
            cout << "FOUND NEW BEST" << endl;
			best = current;
			bestCost = treeCost;
			if (totalCycles != 1)
				cycles = 0;
		} 
		if (cycles % 100 == 0) {
			updatePheromonesGlobal(g, best, false);
		} else {
			updatePheromonesGlobal(g, best, true);
		}
		if (totalCycles % 500 == 0) {
			evap_factor *= P_UPDATE_EVAP; 
			enha_factor *= P_UPDATE_ENHA; 
		}
		totalCycles++;
		cycles++;
		treeCost = 0;
	}
    cout << "Cycles: " << totalCycles << endl;
	cout << bestCost << endl;
	return best;
}

void updatePheromonesGlobal(Graph *g, vector<Edge*> best, bool improved) {
	//	Local Variables
	srand((unsigned) time(NULL));
	double pMax = 1000*((maxCost - minCost) + (maxCost - minCost) / 3);
	double pMin = (maxCost - minCost)/3;
	Edge *e;
	double XMax = 0.3;
	double XMin = 0.1;
	double rand_evap_factor;
	double IP;
	//	For each edge in the best tree update pheromone levels
	for (unsigned int i = 0; i < g->getCount() - 1; i++) {
		e = best[i];
		IP = (maxCost - e->weight) + ((maxCost - minCost) / 3);
		if (improved) {
			//	IMPROVEMENT so Apply Enhancement
			e->pLevel = enha_factor*e->pLevel;
		} else {
			//	NO IMPROVEMENTS so Apply Evaporation
			rand_evap_factor = XMin + rand() * (XMax - XMin) / RAND_MAX;
			e->pLevel = rand_evap_factor*e->pLevel;
		}
		//	Check if fell below minCost or went above maxCost
		if (e->pLevel > pMax) {
			e->pLevel = pMax - IP;
		} else if (e->pLevel < pMin) {
			e->pLevel = pMin + IP;
		}
	}
}

void updatePheromonesPerEdge(Graph *g) {
	//	Local Variables
	Vertex *vertWalkPtr = g->getFirst();
	double pMax = 1000*((maxCost - minCost) + (maxCost - minCost) / 3);
	double pMin = (maxCost - minCost)/3;
	double IP;
	vector<Edge*>::iterator ex;
	Edge *edgeWalkPtr;
	
	while (vertWalkPtr) {
		for ( ex = vertWalkPtr->edges.begin() ; ex < vertWalkPtr->edges.end(); ex++ ) {
			edgeWalkPtr = *ex;
			if (edgeWalkPtr->getSource(NULL) == vertWalkPtr) {
				IP = (maxCost - edgeWalkPtr->weight) + ((maxCost - minCost) / 3);
				edgeWalkPtr->pLevel = (1 - evap_factor)*(edgeWalkPtr->pLevel)+(edgeWalkPtr->pUpdatesNeeded * IP);
				if (edgeWalkPtr->pLevel > pMax) {
					edgeWalkPtr->pLevel = pMax - IP;
				} else if (edgeWalkPtr->pLevel < pMin) {
					edgeWalkPtr->pLevel = pMin + IP;
				}
				//	Done updating this edge reset multiplier
				edgeWalkPtr->pUpdatesNeeded = 0;
			}
		}
		vertWalkPtr = vertWalkPtr->pNextVert;
	}
}

vector<Edge*> treeConstruct(Graph *g, int d) {
    //	Local Variables
    vector<Edge*> v, c, tree, possConn;
    const int HUBS_NEEDED = d - 1;
    vector<Hub*> hubs, treeHubs;
    Vertex *vertWalkPtr, *vert, *v1, *v2;
    vector<Hub*> possVerts;
    Hub *pHub, *h;
    Edge *pE, *pEdge, *edgeWalkPtr;
    int vertIndex;
    int numHubs = 0;
    unsigned int treeCount = 0;
    vector<Edge*>::iterator iedge1, iedge2, iedge3, ie;
    vector<Hub*>::iterator ihubs1, ihubs2;
    Hub *highHub = NULL;
    vector<int> uf( g->getCount()+1 , 0 );
    BinaryHeap* heap;
    //	Put all edges into a vector
    vertWalkPtr = g->getFirst();
    while (vertWalkPtr) {
        vertWalkPtr->treeDegree = 0;
        vertWalkPtr->inTree = false;
        vertWalkPtr->isConn = false;
        for ( ie = vertWalkPtr->edges.begin() ; ie < vertWalkPtr->edges.end(); ie++ ) {
            edgeWalkPtr = *ie;
            //	Dont want duplicate edges in listing
            if (edgeWalkPtr->getSource(NULL) == vertWalkPtr) {
                edgeWalkPtr->inTree = false;
                v.push_back(edgeWalkPtr);
            }
        }
        vertWalkPtr = vertWalkPtr->pNextVert;
    }
    //	Sort edges in ascending order based upon pheromone level
    sort(v.begin(), v.end(), asc_cmp_plevel);
    //for_each(v.begin(), v.end(), printEdge);
    //	Select 5n edges from the end of v( the highest pheromones edges) and put them into c.
    for (unsigned int i = 0; i < 5*g->getCount(); i++) {
        if (v.empty()) {
            break;
        }
        c.push_back(v.back());
        v.pop_back();
    }
    //	Sort edges in descending order based upon cost
    sort(c.begin(), c.end(), des_cmp_cost);
    //for_each(c.begin(), c.end(), printEdge);
    //  Fill vector of Hubs
    vert = g->getFirst();
    for(unsigned int index = 0; index < g->getCount(); index++) {
        hubs.push_back(new Hub());
        hubs[index]->vertId = index + 1;
        hubs[index]->vert = vert;
        vert = vert->pNextVert;
    }
    //cout << "Diameter Bound: " << d << endl;
    //  Now get d - 1 hubs
    while(numHubs < HUBS_NEEDED && treeCount != g->getCount() - 1) {
        if(!c.empty()){
            //  Get Degree of each vertice in candidate set
            for(iedge1 = c.begin(); iedge1 < c.end(); iedge1++) {
                pEdge = *iedge1;
                //  Handle Source
                vertIndex = pEdge->getSource(NULL)->data; // the vertice number uniquely identifies each vertice
                hubs[vertIndex - 1]->edges.push_back(pEdge);
                //  Handle Destination
                vertIndex = pEdge->getDestination(NULL)->data; // the vertice number uniquely identifies each vertice
                //cout << "index: " << vertIndex << endl;
                //cout << "# edges at index " << hubs[vertIndex - 1]->edges.size() << endl;
                hubs[vertIndex - 1]->edges.push_back(pEdge);
            }
            //  Put Potential hubs in to heap
            //  First get rid of vertices with zero edges from candidate set
            for(ihubs2 = hubs.begin(); ihubs2 < hubs.end(); ihubs2++) {
                pHub = *ihubs2;
                if(pHub->edges.size() != 0) {
                    possVerts.push_back(pHub);
                }
            }
            heap = new BinaryHeap( possVerts );
            //  Get highest degree v to make our initial hub (should be top of heap)
            highHub = heap->deleteMax();
            numHubs++;
            treeHubs.push_back(highHub);
            //  Add all edges in highHub to tree
            for(iedge1 = highHub->edges.begin(); iedge1 < highHub->edges.end(); iedge1++) {
                pEdge = *iedge1;
                if(!pEdge->inTree) {
                    pEdge->getDestination(NULL)->inTree = true;
                    pEdge->getSource(NULL)->inTree = true;
                    pEdge->inTree = true;
                    tree.push_back(pEdge);
                    treeCount++;
                }
            }
            //  Update potential connector edges
            if ( numHubs > 1 ) {
                for(int i = numHubs - 1; i >= 1; i--) {
                    v1 = highHub->vert; 
                    v2 = treeHubs[i]->vert;
                    //cout << "v1: " << v1->data << ", v2: " << v2->data << endl;
                    for(iedge3 = v2->edges.begin(); iedge3 < v2->edges.end(); iedge3++) {
                        pEdge = *iedge3;
                        if(pEdge->getDestination(NULL)->data ==  v1->data) {
                            possConn.push_back(pEdge);
                        }
                    }
                }
            }
            //cout << "potential connectors: " << possConn.size() << endl;
                //  Get rid of edges that are already in tree or that would cause a loop
            for(iedge1 = highHub->edges.begin(); iedge1 < highHub->edges.end(); iedge1++) {
                pEdge = *iedge1;
                //cout << pEdge->getDestination(NULL)->data << ", " << pEdge->getSource(NULL)->data << endl;
                //  Update Source Vertex
                h = hubs[pEdge->getSource(NULL)->data - 1];
               // cout << "\n\nSource\n";
                for(iedge2 = h->edges.begin() + 1; iedge2 < h->edges.end(); iedge2++) {
                    pE = *iedge2;
                    //cout << pE->getDestination(NULL)->data << ", " << pE->getSource(NULL)->data << endl;
                    if(pE->getDestination(NULL)->inTree == true && pE->getSource(NULL)->inTree == true) {
                        if(!h->edges.empty()) {
                            h->edges.erase(iedge2);
                        }
                    }
                }
                    //Update Destination
                h = hubs[pEdge->getDestination(NULL)->data - 1];
              //  cout << "\n\nDestination\n";
                    //for_each(h->edges.begin(), h->edges.end(), printEdge);
                for(iedge2 = h->edges.begin() + 1; iedge2 < h->edges.end(); iedge2++) {
                    pE = *iedge2;
                   // cout << pE->getDestination(NULL)->data << ", " << pE->getSource(NULL)->data << endl;
                    if(pE->getDestination(NULL)->inTree == true && pE->getSource(NULL)->inTree == true) {
                        if(!h->edges.empty()) {
                            h->edges.erase(iedge2);
                        }
                    }
                }
            }
            //  Update Heap
            heap->updateHeap();
        } 
        else {
            //	C is empty
            for (unsigned int j = 0; j < 5*g->getCount(); j++) {
                if (v.empty()) {
                    break;
                }
                c.push_back(v.back());
                v.pop_back();
            }
            sort(c.begin(), c.end(), des_cmp_cost);
        }
	}
	//cout << "now trying to connect hubs." << endl;
	//  Now that we have all the hubs we need to connect them.
	sort(possConn.begin(), possConn.end(), asc_cmp_plevel);
	//cout << "sorted possible connections.\n";
	int x1 = 0;
	while(treeCount != g->getCount() - 1 && !possConn.empty()) {
	//  cout << "trying to add edge connector.\n";
		pEdge = possConn.back();
		if(pEdge->getDestination(NULL)->isConn != true && pEdge->getSource(NULL)->isConn != true) {
	        //cout << "Added Connector: " <<  x1++ << " " << endl;
	        // cout << pEdge->getSource(NULL)->data << " " << pEdge->getDestination(NULL)->data << " " << pEdge->weight << " " << pEdge->pLevel << endl;
	        pEdge->getDestination(NULL)->isConn = true;
	        pEdge->getSource(NULL)->isConn = true;
	        tree.push_back(pEdge);
	        treeCount++;
	    //    cout << "added edge connector.\n";
	    }
	    possConn.pop_back();
	}
    //  Return the degree constrained minimum spanning tree
return tree;
}



					  
int findRoot(Vertex* v, vector<int> uf) {
	// find the root 
	int index = v->data;
	int sourceRoot = uf[index];
	while (sourceRoot != index) {
		index = sourceRoot;
		sourceRoot =uf[sourceRoot];
	}
	return index;
}
					  
bool looping(Edge* e, vector<int> uf) {
	if (uf[findRoot(e->getDestination(NULL), uf)] == uf[findRoot(e->getSource(NULL), uf)] && uf[findRoot(e->getSource(NULL), uf)] != 0) {
		return true;
	}
	else {
		return false;
	}
}

void move(Graph *g, Ant *a) {
	Vertex* vertWalkPtr;
	vertWalkPtr = a->location;
	Edge* edgeWalkPtr;
	int numMoves = 0;
	vector<Edge*>::iterator e;
	double sum = 0.0;
	vector<Range> edges;
	double value;
	Range* current;
	vector<int> v = *a->visited;
	//	Determine Ranges for each edge
	for ( e = vertWalkPtr->edges.begin() ; e < vertWalkPtr->edges.end(); e++ ) {
		edgeWalkPtr = *e;
		Range r;
		r.assocEdge = edgeWalkPtr;
		r.low = sum;
		sum += edgeWalkPtr->pLevel + g->getVerticeWeight(edgeWalkPtr->getDestination(vertWalkPtr)); // changed to include destination weight
		r.high = sum;
		edges.push_back(r);
	}
	while (numMoves < 5) {
		//	Select an edge at random and proportional to its pheremone level
		value = fmod(rand(),(sum+1));
		for (unsigned int i = 0; i < edges.size(); i++) {
			current = &edges[i];
			if (value >= current->low && value < current->high) {
				//	We will use this edge
				edgeWalkPtr = current->assocEdge;
				break;
			}
		}
		//	We have a randomly selected edge, if that edges hasnt already been visited by this ant
		if (v[edgeWalkPtr->getDestination(vertWalkPtr)->data] == 0) {
			edgeWalkPtr->pUpdatesNeeded++;
			a->location = edgeWalkPtr->getDestination(vertWalkPtr);
			v[edgeWalkPtr->getDestination(vertWalkPtr)->data] = 1; 
			break;
		} else {
			numMoves++;
		}
	}
}
 
void processFileOld(Graph *g, char* fileName) {
    //  Open file for reading
    ifstream inFile;
    inFile.open(fileName);
    assert(inFile.is_open());
    int eCount, vCount;
    int i, j;
    double cost;
    //  Create each vertex after getting vertex count
    inFile >> vCount;
    for(int i = 1; i <= vCount; i++) {
        g->insertVertex(i);
    }
    //  Create each edge after processing edge count
    eCount = vCount*(vCount-1)/2;
    for(int e = 0; e < eCount; e++) {
        inFile >> i >> j >> cost;
        g->insertEdge(i, j, cost);
		if (cost > maxCost)
			maxCost = cost;
		if (cost < minCost)
			minCost = cost;
    }
}

void processFile(Graph *g, char* fileName) {
    double x,y,cost;
    //  Open file for reading  
    ifstream inFile;
    inFile.open(fileName);
    assert(inFile.is_open());
    int eCount, vCount;
    //  Create each vertex after getting vertex count
    inFile >> vCount;
    for(int i = 1; i <= vCount; i++) {
    	inFile >> x >> y;
        g->insertVertex(i, x, y);
    }
    //  Create each edge after processing edge count
    eCount = vCount*(vCount-1)/2;
    for(int v1 = 1; v1 <= vCount; v1++) {
    	for(int j = 1; v1 + j <= vCount; j++) {
        	cost = g->insertEdge(v1, v1 + j);
			if (cost > maxCost) {
				maxCost = cost;
			}
			if (cost < minCost) {
				minCost = cost;
			}
    	}
	}
}

void processFileNew(Graph *g, char* fileName) {
    double x,y,cost;
    //  Open file for reading  
    ifstream inFile;
    inFile.open(fileName);
    assert(inFile.is_open());
    int eCount, vCount;
    //  Create each vertex after getting vertex count
    inFile >> vCount;
    for(int i = 1; i <= vCount; i++) {
        g->insertVertex(i);
    }
    //  Create each edge after processing edge count
    eCount = vCount*(vCount-1)/2;
    for(int v1 = 1; v1<= vCount; v1++) {
    	for(int j = 1; j <= vCount; j++){
        	inFile >> cost;
        	if(j > v1){
        		g->insertEdge(v1, j, cost);
				if (cost > maxCost)
					maxCost = cost;
				if (cost < minCost)
					minCost = cost;
    		}
   		}
   	}
}
