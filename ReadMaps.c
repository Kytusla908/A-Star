#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct {
    unsigned long id;
    char *name;
    double lat, lon;
    unsigned short nsucc;
    unsigned long *successors;
} node;

unsigned long searchNode(unsigned long id, node *nodes, unsigned long nnodes)
{
    // we know that the nodes where numrically ordered by id, so we can do a binary search.
    unsigned long l = 0, r = nnodes - 1, m;
    while (l <= r)
    {
        m = l + (r - l) / 2;
        if (nodes[m].id == id) return m;
        if (nodes[m].id < id)
            l = m + 1;
        else
            r = m - 1;
    }

    // id not found, we return nnodes+1
    return nnodes+1;
}

// MAIN function
int main(int argc,char *argv[])
{
    clock_t start_time;
    FILE *mapfile;
    unsigned long nnodes;
    char *line=NULL;
    size_t len;

    start_time = clock();

    if(argc<2){
        mapfile = fopen("andorra.csv", "r");
        printf("Opening map andorra.csv.\n");
    }
    else{
        mapfile = fopen(argv[1], "r");
        printf("Opening map %s\n",argv[1]);
    }    if (mapfile == NULL)
    {
        printf("Error when opening the file\n");
        return 1;
    }

    // count the nodes
    nnodes=0UL;
    while (getline(&line, &len, mapfile) != -1){
        if (strncmp(line, "node", 4) == 0){
            nnodes++;
        }
    }
    printf("Total number of nodes is %ld\n", nnodes);
    printf("Elapsed time: %f seconds\n", (float)(clock() - start_time) / CLOCKS_PER_SEC);

    rewind(mapfile);
    
    start_time = clock();
    node *nodes;
    char *tmpline , *field , *ptr;
    unsigned long index=0;

    nodes = (node*) malloc(nnodes*sizeof(node));
    if(nodes==NULL){
        printf("Error when allocating the memory for the nodes\n");
        return 2;
    }

    // Add the data to the "node" datatype structure
    while (getline(&line, &len, mapfile) != -1){
        if (strncmp(line, "#", 1) == 0) continue;
        tmpline = line; // make a copy of line to tmpline to keep the pointer of line
        field = strsep(&tmpline, "|");
        if (strcmp(field, "node") == 0)
        {
            field = strsep(&tmpline, "|");
            nodes[index].id = strtoul(field, &ptr, 10);
            field = strsep(&tmpline, "|");
            strcpy(nodes[index].name,field);
            for (int i = 0; i < 7; i++)
                field = strsep(&tmpline, "|");
            nodes[index].lat = atof(field);
            field = strsep(&tmpline, "|");
            nodes[index].lon = atof(field);

            nodes[index].nsucc = 0; // start every node with 0 successors

            index++;
        }
    }
    printf("Assigned data to %ld nodes\n", index);
    printf("Elapsed time: %f seconds\n", (float)(clock() - start_time) / CLOCKS_PER_SEC);
    printf("Last node has:\n id=%lu\n GPS=(%lf,%lf)\n Name=%s\n",nodes[index-1].id, nodes[index-1].lat, nodes[index-1].lon, nodes[index-1].name);
    
    rewind(mapfile);
    
    start_time = clock();
    int oneway;
    unsigned long nedges = 0, origin, dest, originId, destId;
    while (getline(&line, &len, mapfile) != -1){
        
        if (strncmp(line, "#", 1) == 0) continue;
        tmpline = line; // make a copy of line to tmpline to keep the pointer of line
        field = strsep(&tmpline, "|");

        if (strcmp(field, "way") == 0){
            for (int i = 0; i < 7; i++) field = strsep(&tmpline, "|"); // skip 7 fields
            if (strcmp(field, "") == 0) oneway = 0; // no oneway
            else if (strcmp(field, "oneway") == 0) oneway = 1;
            else continue; // No correct information
            field = strsep(&tmpline, "|"); // skip 1 field
            field = strsep(&tmpline, "|");
            if (field == NULL) continue;
            originId = strtoul(field, &ptr, 10);
            origin = searchNode(originId,nodes,nnodes);

            while(1){
                field = strsep(&tmpline, "|");
                if (field == NULL) break;
                destId = strtoul(field, &ptr, 10);
                dest = searchNode(destId,nodes,nnodes);
                if((origin == nnodes+1)||(dest == nnodes+1)){
                    originId = destId;
                    origin = dest;
                    continue;
                }

                if(origin==dest) continue;

                // Check if the edge did appear in a previous way
                int newdest = 1;
                for(int i=0;i<nodes[origin].nsucc;i++)
                    if(nodes[origin].successors[i]==dest){
                        newdest = 0;
                        break;
                    }
                if(newdest){
                    nodes[origin].successors = realloc(nodes[origin].successors, (nodes[origin].nsucc + 1) * sizeof(unsigned long));
                    nodes[origin].successors[nodes[origin].nsucc]=dest;
                    nodes[origin].nsucc++;
                    nedges++;
                }
                if(!oneway){   
                    // Check if the edge did appear in a previous way
                    int newor = 1;
                    for(int i=0;i<nodes[dest].nsucc;i++)
                        if(nodes[dest].successors[i]==origin){
                            newor = 0;
                            break;
                        }
                    if(newor){
                        nodes[dest].successors = realloc(nodes[dest].successors, (nodes[dest].nsucc + 1) * sizeof(unsigned long));
                        nodes[dest].successors[nodes[dest].nsucc]=origin;
                        nodes[dest].nsucc++;
                        nedges++;
                    }
                }
                originId = destId;
                origin = dest;
            }
        }
    }
    
    fclose(mapfile);

    printf("Total edges added: %lu\n", nedges);
    printf("Elapsed time: %f seconds\n", (float)(clock() - start_time) / CLOCKS_PER_SEC);

    for (unsigned long i = 0; i < nnodes; i++){
        free(nodes[i].successors);  // Free each node's successors array
        free(nodes[i].name);        // Free the duplicated name string
    }

    free(nodes);
    free(line);

    return 0;
}