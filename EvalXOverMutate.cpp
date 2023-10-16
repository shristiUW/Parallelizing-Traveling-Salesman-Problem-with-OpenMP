#include <iostream>  // cout
#include <stdlib.h>  // rand
#include <math.h>    // sqrt, pow   // OpenMP
#include <string.h>  // memset
#include "Timer.h"
#include "Trip.h"   
#include <vector>
#include <omp.h>     // OpenMP


#define CHROMOSOMES    50000 // 50000 different trips
#define CITIES         36    // 36 cities = ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
#define TOP_X          25000 // top optimal 25%
#define MUTATE_RATE    25    // optimal 50%

using namespace std;


// implemented functions
int compare(const void* p1, const void* p2);
int indexMapForDistanceCal(char cityChar);
float calculateDistance(int city1, int city2, int coordinates[CITIES][2]);
void evaluate(Trip trip[CHROMOSOMES], int coordinates[CITIES][2]);
void generateComplement(Trip offsprings[TOP_X], int i);
bool isVisited(const std::vector<bool>& visited, int index);
void setVisited(std::vector<bool>& visited, int index);
void crossover(Trip parents[TOP_X], Trip offsprings[TOP_X], int coordinates[CITIES][2]);
void mutate(Trip offsprings[TOP_X]);


// Implementation of compare function used in sorting
int compare(const void* p1, const void* p2)
{
    float distanceCompare=((Trip*)p1)->fitness -((Trip*)p2)->fitness;
    if(distanceCompare>0)
    {return 1;}
    else if(distanceCompare<0){return -1;}
    else{
        return 0;
    }
}

//Function to find out the indexes of a city
int indexMapForDistanceCal(char cityChar)
{
   return (cityChar >= 'A' && cityChar <= 'Z') ? (cityChar - 'A') : (cityChar - '0' + 26);
} 


// Function to calculate distance between two cities
float calculateDistance(int city1, int city2, int coordinates[CITIES][2])
{
    float distance = sqrt(pow(coordinates[city1][0]-coordinates[city2][0],2)+pow(coordinates[city1][1]-coordinates[city2][1],2));
    return distance;
}



// Function to evaluate fitness of trips
void evaluate(Trip trip[CHROMOSOMES], int coordinates[CITIES][2]) {   
    // Cache for storing distances of all cities
    float distanceCache[CITIES][CITIES];
 
    for (int i = 0; i < CITIES; ++i) {
        for (int j = 0; j < CITIES; ++j) {
            distanceCache[i][j] = calculateDistance(i, j, coordinates);
        }
    }

    float totalDistance=0.0;
    //Use parallelization - using reduction for totalDistance
    #pragma omp parallel for reduction(+:totalDistance) 
    for (int i = 0; i < CHROMOSOMES; ++i) {
        totalDistance = 0.0;
        char* itinerary = trip[i].itinerary;

        // Add distance from 0,0 to first city
        int indexCity1 = indexMapForDistanceCal(itinerary[0]);
        totalDistance += sqrt(pow(coordinates[indexCity1][0], 2) + pow(coordinates[indexCity1][1], 2));

        // Calculate distance for all cities of itinerary
        for (int j = 1; j < CITIES; ++j) {
            int indexCity2 = indexMapForDistanceCal(itinerary[j]);
            totalDistance += distanceCache[indexCity1][indexCity2];
            indexCity1 = indexCity2;  // Update indexCity1
        }

        trip[i].fitness = totalDistance;
    }

    // Sort the trips based on fitness
    qsort(trip, CHROMOSOMES, sizeof(Trip), compare);
    
  
}


// Function to generate complement of offspring for generating next offspring
void generateComplement(Trip offsprings[TOP_X], int i)
{
 
    char complementCityList[CITIES] ={'9', '8', '7', '6', '5', '4', '3', '2','1', '0', 'Z', 'Y', 'X', 'W', 'V', 'U', 
    'T', 'S', 'R', 'Q', 'P', 'O', 'N', 'M', 'L', 'K', 'J', 'I', 'H', 'G', 'F', 'E', 'D', 'C', 'B', 'A'};
    int j;
     
    for(int j=0;j<CITIES; j++)
    {
        int origOffSpring= indexMapForDistanceCal(offsprings[i].itinerary[j]);
        offsprings[i+1].itinerary[j]= complementCityList[origOffSpring];

    }
    
}

// Function to check if a city is visited
bool isVisited( const std:: vector<bool>& visited,int index)
{
      if (index >= 0 && index < 36) {
        return visited.at(index);
    } else {
        std::cout << "Index out of range." << std::endl;
        return false;  
    }

}

// Function to set a city as visited
void setVisited(std:: vector<bool>& visited, int index)
{
    if(index>=0 && index< CITIES)
    {
        visited.at(index) = true;    }
        else{
              std::cout << "Index out of range." << std::endl;
        }
}

// Function for crossover of parents to generate offspring
void crossover( Trip parents[TOP_X], Trip offsprings[TOP_X], int coordinates[CITIES][2]) {  
    //Caching distances
    float distanceCache[CITIES][CITIES];
    for (int i = 0; i < CITIES; ++i) {
        for (int j = 0; j < CITIES; ++j) {
            distanceCache[i][j] = calculateDistance(i, j, coordinates);
        }
    }
    // OMP Parallelization - putting currentCity in private 
    char currentCity;
    #pragma omp parallel for private( currentCity )
    for(int i=0;i<TOP_X;i+=2)
    {
        char* parent1= parents[i].itinerary;
        char* parent2 =parents[i+1].itinerary;
        
        //Keep track of visited cities and offspring itineraries is because we need unique cities
        std::vector<bool> visited(CITIES, false);         
        //First city visited is 0th element of parent1
        setVisited(visited,indexMapForDistanceCal(parent1[0]));
        currentCity=parent1[0];
        //set current city into offspring[i];
        offsprings[i].itinerary[0]=currentCity;
       
        //Loop for every city
        for(int j=1; j<CITIES; j++)
        {    
            int parent1Index= indexMapForDistanceCal(currentCity);                   
            int parent1NextIndex=0; 

            //find current city in parent1 itinerary
            while(parent1NextIndex<CITIES && parent1[parent1NextIndex] !=currentCity)
            {
                parent1NextIndex++;
            }
        
            //Looping back to zero of it crosses 35th index
            parent1NextIndex = (parent1NextIndex + 1 == CITIES) ? 0 : parent1NextIndex + 1;

            // created a temporary variable to store ordiginal next index in case all cities have been visited
            int originalParent1NextIndex = parent1NextIndex;  
            //Flag to set if all cities are visited
            bool flag1 = true;  

            //check visited check
            while(isVisited(visited,indexMapForDistanceCal(parent1[parent1NextIndex])))
            {
                //The city has been visited
                parent1NextIndex++;
                if(parent1NextIndex>=CITIES)
                {
                    parent1NextIndex=0;
                }
            
                //check if we have looped back to the original index
                if(parent1NextIndex == originalParent1NextIndex)
                {
                    //All cities have been visited
                flag1=false;
                    break;
                }
            } 
                            
            //Do the same for parent 2
            int parent2Index= indexMapForDistanceCal(currentCity);
            int parent2NextIndex=0;
        
            while(parent2NextIndex<CITIES && parent2[parent2NextIndex] !=currentCity)
            {
                parent2NextIndex++;
            }

            //for finding city next to current city in parent2
            parent2NextIndex = (parent2NextIndex + 1 == CITIES) ? 0 : parent2NextIndex + 1;

            // created a temporary variable to store ordiginal next index in case all cities have been visited 
            int originalParent2NextIndex = parent2NextIndex;  
            bool flag2= true;

            //find next unvisited city in parent2
            while(isVisited(visited,indexMapForDistanceCal(parent2[parent2NextIndex])))
                {
                    //The city has been visited
                    parent2NextIndex++;
                    if(parent2NextIndex>=CITIES)
                    {
                        parent2NextIndex=0;
                    }                       
                    //check if we have looped back to the original index
                    if(parent2NextIndex == originalParent2NextIndex)
                    {
                        //All cities have been visited
                        flag2=false;
                        break;
                    }
                }  
                //calculate distance
                if(flag1 && flag2)
                {
                float dist1=0;
                if(parent1NextIndex<CITIES)
                {
                dist1= distanceCache[parent1Index][indexMapForDistanceCal(parent1[parent1NextIndex])];
                }
                float dist2=0;
                if(parent2NextIndex<CITIES)
                {                   
                dist2= distanceCache[parent2Index][indexMapForDistanceCal(parent2[parent2NextIndex])];                       
                }
            
                //update currentCity and offspring 1 based on short distance
                if(dist1<dist2)
                {
                    //update current city
                    currentCity=parent1[parent1NextIndex];
                    visited[indexMapForDistanceCal(currentCity)]=true;
                    offsprings[i].itinerary[j]=currentCity;                        
                }
                else
                {
                    currentCity = parent2[parent2NextIndex];
                    visited[indexMapForDistanceCal(currentCity)]=true;
                    offsprings[i].itinerary[j]= currentCity; 
                }                  
                }
    }

    //generate offsprings[i+1]
    generateComplement(offsprings,i);                             
    }
       
}

 //Function to mutate a pair of genes in each offspring.
 void mutate( Trip offsprings[TOP_X] ) {
 
    //No parallelization for this function as it is impacting performance when we provide 1 thread 
    for(int i=0; i< TOP_X; i++)
    {
        if (rand() % 100 < MUTATE_RATE )
        {
        int city1= rand() % CITIES;
        int city2 = rand() % CITIES;
        while(city1==city2)
        {
         city1= rand() % CITIES;
        }
        swap(offsprings[i].itinerary[city1], offsprings[i].itinerary[city2]);
        }
    }

}



