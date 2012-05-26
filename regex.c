#include "regex.h"



/*
*	parsegroup
*	
*/

char* createtoken(unsigned int size, char* from){
	char* nt;

	nt = (char *) malloc (size*sizeof(char));
	memset(nt,0,size);
	strncpy(nt,from,size-1);
	return nt;

}
int parsegroup(char* token,char ** groups)
{
	int nbdifferentfield=0;
	char* dot;
	char* nextdot;
	char* nextbrack;
	unsigned int ntsize = 0;

	
	dot = strchr(token,'.');
	while(dot != NULL){
		nextbrack = strchr(dot,'}');
		nextdot=strchr(dot+1,'.');
		if(dot != token){
			ntsize = (unsigned int)(dot-token+1);
			groups[nbdifferentfield] = createtoken(ntsize,token);
			nbdifferentfield++;
			token = dot;
		}
		else{
			//There are no more variable fields
			if(nextdot == NULL){
					if(nextbrack==NULL){
						ntsize = 2;
						groups[nbdifferentfield] = createtoken(ntsize,token);
						nbdifferentfield++;
						token = dot+1;
					}
					else{
						ntsize = (unsigned int)(nextbrack-dot+2);
						groups[nbdifferentfield] = createtoken(ntsize,token);
						nbdifferentfield++;
						token = nextbrack+1;
					}
			}
			//There exist other variable fields
			else{
					if(nextbrack==NULL || (nextbrack >= nextdot)){
						ntsize = 2;
						groups[nbdifferentfield] = createtoken(ntsize,token);
						nbdifferentfield++;
						token = dot+1;
					}				
					else{
						ntsize = (unsigned int)(nextbrack-dot+2);
						groups[nbdifferentfield] = createtoken(ntsize,token);
						nbdifferentfield++;
						token = nextbrack+1;
						
					}
					ntsize = (unsigned int)(nextdot - token + 1);
					if(ntsize>1){
						groups[nbdifferentfield] = createtoken(ntsize,token);
						nbdifferentfield++;
					}
					token = nextdot;
			}
			dot = nextdot;
		}
		
	}
	if(strlen(token)>0){
		ntsize = (unsigned int)(strlen(token)+ 1);
		groups[nbdifferentfield] = createtoken(ntsize,token);
		nbdifferentfield++;
	}
	return nbdifferentfield;
}

/*
	rollBack
	*********************************************
	shift:	
	ind: 	index of the previous variable field

	.......aaaaaa................bbbbbbb
		^				^
		|				|
	previous		too long
*/
int rollBack(unsigned int shift, int ind,Fields* fields, char* tomatch, int first, int lastvar)
{
	char* nextmatching;
	Fields* stat = (&fields[ind+1]);
	Fields* var = (&fields[ind]);
	int retvalue = 0;
	if(first){
		if(ind >= 2){
			while(1){
                if(!lastvar)
				    shift = (unsigned int)(stat->add - var->add - var->max);
				retvalue = rollBack(shift, ind - 2,fields,tomatch,0,0);
				if( retvalue == 0){
					var->add = fields[ind-1].add + fields[ind-1].len;
                    if(!lastvar){
				    	if(stat->add - var->add >= var->min){
				    		var->len = (unsigned int)(stat->add - var->add);
				    		return 0;
				    	}
				    	else{
				    		nextmatching = strstr(var->add + var->min, stat->value);
				    		if (nextmatching == NULL){
				    			return -1;
				    		}
				    		else{
				    			stat->add = nextmatching;
				    			var->len = (unsigned int)(nextmatching - var->add);
				    			if(var->len<=var->max)
				    				return 0;
				    		}
				    	}
                    }
                    else{
                        if(strlen(stat->add) >= var->min){
				    		var->len = (unsigned int)(strlen(stat->add));
				    		return 0;
				    	}
                        else{
                            return -1;
                        }
                    }
				}
				else
					return -1;
			}
		}
		else{
			return -1;
		}
	}
	else{
		nextmatching = strstr(stat->add + shift, stat->value);
		// Cannot shift the previous static field
		if (nextmatching == NULL){
			return -1;
		}
		// The next matching of aaaaa is such as the len of the variable field is lower than the max authorized
		else if((unsigned int)(nextmatching - var->add)<= var->max){
			stat->add = nextmatching;
			var->len = (unsigned int)(nextmatching - var->add);
			return 0;
		}
		else if(ind >= 2){
			shift = (unsigned int)(nextmatching - var->add - var->max);
			retvalue = rollBack(shift, ind - 2,fields,tomatch,0,0);
			if( retvalue == 0){
				var->add = fields[ind-1].add + fields[ind-1].len;
				if(nextmatching - var->add >= var->min){
					stat->add = nextmatching;
					var->len = (unsigned int)(nextmatching - var->add);
					return 0;
				}
				else{
					nextmatching = strstr(var->add + var->min, stat->value);
					if (nextmatching == NULL){
						return -1;
					}
					else{
						stat->add = nextmatching;
						var->len = (unsigned int)(nextmatching - var->add);
						return 0;
					}
				}
			}
			else
				return -1;
		}
		else
			return -1;
	}
	return 0;
}



/*	
	match
	*********************************************
	return: 
		ind = last field index
		-1 : "format error"
		-2 : "not matching"


*/
int match(char* regex,char* tomatch,Fields* fields,int* groupindex){
	#define isgood(c) isalnum(c)||c==46

	char delim='.';
	char border[]="()";
	int ind=0;
	unsigned int size=0;
	char* posmatch;
	int maxlimit = strlen(tomatch);
	int rollret = 0;
	char* tomatchcopy = tomatch;
	int retvalue =0;
	char* tempgroup;
	char *str1;
	char* token;
	char* groups[MaxFields];
	int nbsubtoken = 0;
	int i;
	if(strlen(regex)<=0)
		return -4;
	if(strlen(tomatch)<=0)
		return -5;
	for(str1 = regex;;str1 = NULL){
		tempgroup = strtok(str1,border);
		if (tempgroup==NULL)
			break;
		
		nbsubtoken = parsegroup(tempgroup,groups);
		if(nbsubtoken<=0){
			freeFields(fields,ind+1);
			return -1;
		}
		
		for(i=0;i<nbsubtoken;i++){
		    token=groups[i];
		//printf("substring %s\n",token);
    		//Very first field
	    	if(fields[ind].set==0){
		    	retvalue = newField(&fields[ind],isStatic(token,'.'),tomatch,token,maxlimit,*groupindex);
			    if(retvalue<0){
		    		freeFields(fields,ind+1);
		    		return retvalue;
		    	}
		    	size+=strlen(token);
		    }

		
    		//New type of field
    		else if(fields[ind].isStatic != isStatic(token,'.')){
    			if(fields[ind].isStatic){
    				setFieldvalue(&fields[ind]);
    				if(ind==0){
    					posmatch = strstr(tomatch,fields[ind].value);
    					if(posmatch!=tomatch){
    						freeFields(fields,ind+1);
    						return -2;
    					}
    					else{
    						setAdd(&fields[ind],posmatch);
	    					tomatch = posmatch + fields[ind].len;
    					}
    				}
    				else{	
						posmatch = strstr(tomatch+fields[ind-1].min,fields[ind].value);
    					if(posmatch == NULL){
						   	freeFields(fields,ind+1);
							return -2;
						}
						else if((unsigned int)(posmatch-tomatch)>fields[ind-1].max){
							rollret = rollBack(0, ind-1,fields,tomatchcopy, 1,0);
							if(rollret!=0){
								freeFields(fields,ind+1);
								return -2;
							}
							//fields[ind-1].len = (int)(fields[ind].add-fields[ind-1].add);
							tomatch = fields[ind].add+fields[ind].len;
						}
						else{
							fields[ind-1].len = (int)(posmatch-tomatch);
							setAdd(&fields[ind],posmatch);
							tomatch = posmatch + fields[ind].len;
						}
				    }
			    }
		    	size = 0;
		    	ind++;
		    	if(ind>=MaxFields-1){
		    		freeFields(fields,ind);
		    		return -3;
	    		}
		    	retvalue = newField(&fields[ind],isStatic(token,'.'),tomatch,token,maxlimit,*groupindex);
	    		if(retvalue<0){
		    		freeFields(fields,ind+1);
		    		return retvalue;
		    	}
		    	size+=strlen(token);
		    }

	    	//Same type of field => push the subfield in the chainlist 
	    	else{
		    	retvalue = addSubfield(&fields[ind],token,maxlimit,*groupindex);
		    	if(retvalue<0){
				freeFields(fields,ind+1);
		        		return retvalue;
		    	}
		    	size+=strlen(token);
		    }
		}

		++*groupindex;
	}
	/*******************************************************/
	//last field
	//Last field is static
	if(fields[ind].isStatic){
		setFieldvalue(&fields[ind]);
		if(ind==0){
			posmatch = strstr(tomatch,fields[ind].value);
			if(posmatch!=tomatch){
				freeFields(fields,ind+1);
				return -2;
			}
			else{
				setAdd(&fields[ind],posmatch);
				tomatch = posmatch + fields[ind].len;
			}
		}
		else{	
				posmatch = strstr(tomatch+fields[ind-1].min,fields[ind].value);
				if(posmatch == NULL){
					freeFields(fields,ind+1);
					return -2;
				}
                    
                if(strcmp(tomatch+(strlen(tomatch)-strlen(fields[ind].value)), fields[ind].value) != 0){
                    freeFields(fields,ind+1);
					return -2;
                }
                else{
                    posmatch = tomatch+(strlen(tomatch)-strlen(fields[ind].value));
                    setAdd(&fields[ind],posmatch);
                }
                printf("%s\n",tomatch+(strlen(tomatch)-strlen(fields[ind].value)));
				if((unsigned int)(posmatch-tomatch)>fields[ind-1].max){
                    printf("in\n");
					rollret = rollBack(0, ind-1,fields,tomatchcopy, 1,0);
					if(rollret!=0){
						freeFields(fields,ind+1);
						return -2;
					}
					//fields[ind-1].len = (int)(fields[ind].add-fields[ind-1].add);
					tomatch = fields[ind].add+fields[ind].len;
				}
				else{
					fields[ind-1].len = (int)(posmatch-tomatch);
					setAdd(&fields[ind],posmatch);
					tomatch = posmatch + fields[ind].len;
				}
		}
	}
	//Last field is dynamic
	else{
        fields[ind].len =strlen(tomatch);
		if(strlen(tomatch)>fields[ind].max){
            rollret = rollBack(strlen(tomatch)-fields[ind].max, ind,fields,tomatchcopy, 1,1);
            if(rollret!=0){
               freeFields(fields,ind+1);
				return -2; 
            }
        }
		else if (strlen(tomatch)<fields[ind].min){
            freeFields(fields,ind+1);
			return -2;
		}

	}
	freeFields(fields,ind+1);
	return ind;
}

/*
int main(int argc, char** argv){
	int indFields;
	char* tomatch;
	char* regex;
	Fields fields[1024];
	if (argc ==3){
		tomatch = argv[2];
		regex = argv[1];
		indFields = match(regex,tomatch,fields);
		if(indFields>=0){
			//printf("GOOD\n");
			while (indFields>=0){
				////printf("%ld\n",(fields[indFields].len)/2);
				////printf("%s\n",fields[indFields].add);
				indFields--;
			}
			////printf("%s\n",tomatch);
		}
		else
			//printf("Echec\n");

	}
	return 0;
}*/
