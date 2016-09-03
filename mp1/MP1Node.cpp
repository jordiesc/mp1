/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;

}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
    	log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = *(int*)(&memberNode->addr.addr);
	int port = *(short*)(&memberNode->addr.addr[4]);

	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    initMemberListTable(memberNode);

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group

    	cout << "en metodo introduceSerfToGroup"<< endl;

    	log->LOG(&memberNode->addr, "Starting up group...");
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr) + sizeof(joinaddr->addr) + sizeof(long) + 1;
        msg = (MessageHdr *) malloc(msgsize * sizeof(char));

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));

#ifdef DEBUGLOG

        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
	vector<MemberListEntry> memberList;
	 MessageHdr *msg = (MessageHdr *) data;
	 //Address * myAdress = (Address *) env;
	// log->LOG(myAdress,"insertando me");

	   Address *joiningAddr = (Address *) malloc(sizeof(Address));
	   memcpy(&joiningAddr->addr, ((char *)msg) + sizeof(MessageHdr), sizeof(joiningAddr->addr));

	   printf("Received mensaje  from: ");
	   printAddress(joiningAddr);
	   cout << "yo soy "<< this->memberNode->addr.getAddress() << endl;


		memberList = this->memberNode->memberList;


	 if (msg->msgType == JOINREQ) {



		cout << "menjsaje de tipo JOINREQ" << endl;
       //update mi lista como es un inicio no hace falta buscar el hearbit();
	   MemberListEntry miembro ;

	   int sizedelVector = memberList.size();

	   cout << "dimension del vector para empezar : " << sizedelVector << endl;
	   bool myselft = false;
	  bool insertadoOexistente;
	  insertadoOexistente = false;


		int myId= getId(this->memberNode->addr.addr);

		vector<MemberListEntry>::iterator it;
		int i =0;
		for(it=memberList.begin() ; it < memberList.end() ; it++,i++ ) {
			myId = getId(joiningAddr->addr);
		    if(it->id == myId){
		    	    insertadoOexistente =  true;
		    		it->heartbeat ++ ;
		    		it->timestamp=this->par->getcurrtime();
		    	}
		    }


	   if ( !insertadoOexistente){
		   cout << "no exite lo cre" << endl;
	    	insertadoOexistente = true;
	    	int currentime = this->par->getcurrtime();

	    	int direc2= getId(joiningAddr->addr);

	    	if(! isMarkedFailed(direc2)){
	    	//if(true){
	    		cout << "creo una nueva entrada" << endl;
				MemberListEntry nuevo = MemberListEntry(direc2,0,0,currentime);
				memberList.push_back(nuevo);

				cout << "insertado elemento " << direc2 << endl;
				log->logNodeAdd(&this->memberNode->addr,joiningAddr);
	    	}


	   }

	    // adding my self into the list.
		//int id = *(int*)(&memberNode->addr.addr);
		int id = getId(memberNode->addr.addr);
		int port = *(short*)(&memberNode->addr.addr[4]);

	    int sizelista = memberList.size();
	   bool exist = false;
	   for (int i = 0 ; i <sizelista ; i++){
		  if(id == memberList.at(i).id)
		   exist = true;
	   }
	   if(!exist){
        cout << "NO EXISTO Y ME CREO" << endl;
		Address direccion= Address(to_string(id)+":0:0:"+to_string(port));
		MemberListEntry newentrada = MemberListEntry(id,port,0,this->par->getcurrtime());
		memberList.push_back(newentrada);
		log->logNodeAdd(&direccion,&direccion);

	   }

	   this->memberNode->memberList = memberList;


	   enviarLista(2,&this->memberNode->addr, joiningAddr);



	 }
     if (msg->msgType == JOINREP) {
    	 cout << "menjsaje de tipo JOINREP" << size << endl;

    	 string mensaje;
    	 this->memberNode->inGroup = true;

    	 //como me ha respondedio el emisor quiere decir que esa.
    	 checkJoinedAdrress(joiningAddr);
    	 mergeWihtOwnLista(true,data,size);


     }

     if (msg->msgType == PINGREQ) {
    	 //merge mi lista teh jar

    	 cout << "en PINGREQ" << size<< endl;
    	 if (size > 15){
    	 mergeWihtOwnLista(false,data,size);
    	 msg->msgType= PINGRER;
    	 enviarLista(4,&this->memberNode->addr, joiningAddr);
    	 }

     }
     if (msg->msgType == PINGRER){

    	 cout << "en PINGRER" << size << endl;
    	 if (size > 15) {
    	 //se revisa los elementos viejos y se borran
    	 checkJoinedAdrress(joiningAddr);
    	 mergeWihtOwnLista(false,data,size);
    	// reviewOldMember()
    	 }
     }




	return true;




}//end method

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	vector<MemberListEntry> memberList;


	/*
	 * Your code goes here
	 */
	int me;
	bool estoy = false;
	cout << "en  funcion nodeLoopsOps" <<endl;
	memberList = this->memberNode->memberList;
	cout << "debug 1 " <<endl;
   int numeroElementos = memberList.size();
   cout << "debug 2 numero elementos" << numeroElementos <<endl;
   int gosipchoosed = rand() % (numeroElementos);
   cout << "debug 3 " <<endl;
   cout << "gosip peer " << gosipchoosed << "mumero elementos"<< numeroElementos << endl;


   MemberListEntry entry = memberList.at(gosipchoosed);
   cout << "exception" << endl;
   this->memberNode->heartbeat ++;
   for (int i=0; i< numeroElementos ; i++){

	   string a;
	   string b;


	   MemberListEntry entrada = memberList.at(i);


	    //me = *(int*)(&memberNode->addr.addr[0]);
	   me = getId(memberNode->addr.addr);
	   if(me == entrada.id) {

		   cout << "aumento el heartbeat de " << me << endl;
		   //entrada.heartbeat = entrada.heartbeat+1;
		   cout << "heartbeat " << entrada.heartbeat << endl;
		   estoy= true;
	   }
   }

   //cout << " estoy vivo y me creo" << me << endl;
   if (!estoy){
	   //como estoy vivo me creo
	   MemberListEntry entrada = MemberListEntry(me,0,0,this->par->getcurrtime());
	   memberList.push_back(entrada);
	   log->logNodeAdd(&memberNode->addr,&memberNode->addr);
   }

  if (entry.id ==10)
	  Address direccion= Address("10:0:0:"+to_string(entry.port));

   Address direccion= Address(to_string(entry.id)+":0:0:"+to_string(entry.port));
   this->memberNode->memberList = memberList;
   //printList();
   updateHeartBeat();
   //printList();
   delteOldPeersOrMarkFailed();
   enviarLista(3,&this->memberNode->addr, &direccion );

  // delteOldPeersOrMarkFailed();

}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}



void MP1Node::enviarLista(int tipo,Address *from, Address *to ){


			vector<MemberListEntry> memberList;
			MessageHdr * mensaje;
			char * datos ;
			string result ;
			 //  printList();
			memberList = this->memberNode->memberList;

			  // printList();

			int sizedelVector = memberList.size();
			cout << " enviando lista tampaño " << sizedelVector << endl;
			result.append("");

			//cout << "Een metodo enviar Lista" << sizedelVector << endl;

			for (int i=0 ; i < sizedelVector ; i++){
		 	 //  MemberListEntry  * miembro ;


		 	  MemberListEntry  miembro = memberList.at(i);

		      int pid = miembro.id;
		      short port = miembro.port;
		      long heartbeat = miembro.heartbeat;
		      cout << "heartbeat" << heartbeat << endl;
		      long timestamp = miembro.timestamp;


		      result = result + to_string(pid)+"I"+to_string(port)+"P"+to_string(heartbeat)+"H"+to_string(timestamp)+"T";

		      cout << "datos to sent "<< pid <<":" << port <<":" << heartbeat<<":" << timestamp << "ENDELEMENT" << endl;

		    }

			result= result+"\n";

			//datos = (char *) malloc()



			mensaje = (MessageHdr *) malloc(sizeof(MessageHdr)+sizeof(Address)+result.size());
			if (tipo == 1)
			mensaje->msgType= JOINREQ;
			if (tipo ==2)
			mensaje->msgType= JOINREP;
			if( tipo == 3)
			mensaje->msgType= PINGREQ;
			if( tipo == 4)
			mensaje->msgType= PINGRER;


			datos = (char *) mensaje;
			memcpy (datos,(char *) &mensaje->msgType,sizeof(MessageHdr));
			memcpy (datos+sizeof(MessageHdr),&memberNode->addr.addr,sizeof(Address));
			memcpy( datos+sizeof(MessageHdr)+sizeof(Address),result.c_str(),result.size());

			//cout << "lo que se envia es"<< result <<endl;
			this->memberNode->memberList = memberList;

		    this->emulNet->ENsend(&memberNode->addr,to,datos,sizeof(MessageHdr)+sizeof(Address)+result.size());

		    //cout << "enviado mensaje a " << endl;
		    //printAddress(to);
		    free(datos);


		}
		/**
		 *
		 */
		void MP1Node::mergeWithOwnLIsta(bool mode, int id, short port, long heartbeat,long tiemstamp){

			cout << "merge con mis lista y los valores" << "datos to sent " << id <<":" << port <<":" << heartbeat<<":" << tiemstamp << "" << endl;
			//this->printList();

			vector<MemberListEntry> memberList;
			memberList = this->memberNode->memberList;
			bool encontrado = false;
			int sizedelVector = memberList.size();


			for (int i=0 ; i < sizedelVector ; i++){

				MemberListEntry  miembro = memberList.at(i);
				if(id == miembro.id){
					encontrado = true;
					if (heartbeat > miembro.heartbeat){
						cout << "actaualizo el heartbeat" << id << "hearbeat del id " << heartbeat << endl;
						miembro.setheartbeat(heartbeat);
						miembro.settimestamp(this->par->getcurrtime());

						MemberListEntry newEntry = miembro;

						memberList.erase(memberList.begin()+i);
						memberList.push_back(newEntry);




					}

				}
			}
			cout << "MODE       "<< mode << endl;
			if(!encontrado & mode){

					Address direccion= Address(to_string(id)+":0:0:"+to_string(port));

					MemberListEntry newentrada = MemberListEntry(id,port,0,tiemstamp);
					memberList.push_back(newentrada);
					log->logNodeAdd(&this->memberNode->addr,&direccion);



			}

			this->memberNode->memberList = memberList;
			//cout << " valores despues del merge" << endl;
			//this->printList();

		}

		/**
		 *
		 */

		void MP1Node::mergeWihtOwnLista( bool mode, char *data, int size){

			MessageHdr * mensajeHeader;
			Address * direccion;
			char * temp;

			temp = (char *)malloc(2042*sizeof(char));
			char buff[2042];

			//cout << "Merging the recieved list with the own list" << endl;
			cout << "MODE mergeWithOwnLista" << size << endl;

			mensajeHeader  = (MessageHdr *)data;
			direccion = (Address *) malloc(sizeof(Address));
			memcpy(direccion,data+sizeof(MessageHdr),sizeof(Address));
			//printAddress(direccion);
			//string datos(data+sizeof(MessageHdr)+sizeof(Address),size-(sizeof(MessageHdr)+sizeof(Address)));
			int i=0;
			while(  i < size-(sizeof(Address)+sizeof(MessageHdr))){

				memcpy(&buff[i],data+sizeof(MessageHdr)+sizeof(Address)+i,sizeof(char));
				//cout << buff[i];
				i++;

			}

			cout << buff << endl;
			//cout<< endl;
			//((dimension of the buffer is i


			int j=0;
			while(j<i){


				string id;
				string port;
				string heartbite;
				string timestamp;

				int iid;
				short pport;
				long lheartbeat;
				long ltiemstamp;

				while(buff[j] != 'I'){
					id = id+buff[j];
					j++;
				}
				j++;
				while(buff[j] != 'P'){
					port = port + buff[j];
					j++;
				}
				j++;
				while(buff[j] != 'H'){
					heartbite = heartbite + buff[j];
					j++;
				}
				j++;
				while(buff[j] != 'T'){
					timestamp = timestamp + buff[j];
					j++;
				}
				j++;

				cout << "sacand churro ";
				cout << id <<":" << port <<":" << heartbite << ":" << timestamp << endl;
				iid =stoi(id);
				pport= (short) stoi(port);
				lheartbeat = stol(heartbite);
				ltiemstamp = stol(timestamp);

				if (buff[j]=='\n')
					break;

				mergeWithOwnLIsta(mode,iid,pport,lheartbeat,ltiemstamp);


			}



			free(temp);



		}


		int MP1Node::getId( Address * idadress){
			int id = 0;
			memcpy(&id, &idadress->addr[0], sizeof(int));
			return id;
		}

		int MP1Node::getId( char address[6] ){
			int id = 0;
			memcpy(&id,address , sizeof(int));
			return id;
		}
		/**
		 * la adress ha respondido eso quiere decir que debe  incluida
		 */

		void MP1Node::checkJoinedAdrress(Address * directionsanswer){

			vector<MemberListEntry> memberList;
			memberList = this->memberNode->memberList;

			int sizedelVector = memberList.size();
			bool encontrado = false;

			for (int i=0 ; i < sizedelVector ; i++){

				MemberListEntry  miembro = memberList.at(i);
				int idamswer= getId(directionsanswer);

				if(idamswer == miembro.id){
					encontrado = true;
					miembro.timestamp = this->par->getcurrtime();

				}
			}

			if(!encontrado){
				int idamswer= getId(directionsanswer);

				if(! isMarkedFailed(idamswer)){
					MemberListEntry newentrada = MemberListEntry(idamswer,0,0,this->par->getcurrtime());
					memberList.push_back(newentrada);
					newentrada.timestamp = this->par->getcurrtime();
					log->logNodeAdd(&this->memberNode->addr,directionsanswer);
				}
			}
			this->memberNode->memberList = memberList;



		}
		/**
		 * delete old peers
		 */

		void MP1Node::delteOldPeersOrMarkFailed(){

			vector<MemberListEntry> memberList;
			memberList = this->memberNode->memberList;

			int vectorsizeInitial = memberList.size();
			for (int i=0 ; i < vectorsizeInitial ; i++){

				int nuevatalla = memberList.size();
				for (int j= 0; j <nuevatalla ; j++ ){

					MemberListEntry  miembro = memberList.at(j);
					int id = miembro.id;
					short port = miembro.port;
					//cout<< "debug deletOldPeers"<<id <<port <<endl;

					//Address direccion= Address(to_string(id)+":0:0:"+to_string(port));



					long diferenceTime=  this->par->getcurrtime() - miembro.timestamp ;

					//cout<< "Para marcar TFAIL id" << id << "tiempo" << diferenceTime ;
					if (diferenceTime> TFAIL){
						//cout << " markado " << id << endl;
						//marked as a Failed so never joined again;
						this->failedvector.push_back(id);
					}

					if( diferenceTime> TREMOVE ){

						if( isMarkedFailed(id)){

							Address direccion;
							memcpy(&direccion.addr[0],&id, sizeof(int));
							memcpy(&direccion.addr[4], &port, sizeof(short));

							cout << "se borra el nodo con tiempo y dierencia "<< id << "times" << miembro.timestamp << "diferencia tiempo "<<diferenceTime << "tiempo[]" << this->par->getcurrtime() << endl;
							log->logNodeRemove(&memberNode->addr,&direccion);

							memberList.erase(memberList.begin()+j);
							break;
						}

					}
			    }

			}

			this->memberNode->memberList = memberList;
		}
		/**
		 * verifica que esta fallado
		 */


		bool MP1Node::isMarkedFailed(int id){
			cout << "en marked failed" << endl;
			bool exist = false;
			int size = this->failedvector.size();
			for (int i=0; i < size; i ++ ){
				if ( id ==this->failedvector[i]){
					exist = true;
					break;
				}
			}
			return exist;
		}
		/**
		 * update the herartbeat
		 *
		 */

		void MP1Node::updateHeartBeat(){

			int myId= getId(this->memberNode->addr.addr);

			updateHeartBeat(myId);




		}
		/**
		 *
		 */

		void MP1Node::updateHeartBeat (int id){

			vector<MemberListEntry> memberList;
			memberList = this->memberNode->memberList;


			vector<MemberListEntry>::iterator it;
			int i =0;
			for(it=memberList.begin() ; it < memberList.end() ; it++,i++ ) {

			    	if(it->id == id){
			    		it->heartbeat++ ;
			    		it->timestamp=this->par->getcurrtime();
			    	}
			    }


			this->memberNode->memberList = memberList;


		}

		void MP1Node::printList (){

			vector<MemberListEntry> memberList;
			memberList = this->memberNode->memberList;

			int size = memberList.size();

			cout << "LISTA A ENVIAR " << size << endl;

			for (int i =0; i< size ; i++){
				MemberListEntry mem = memberList.at(i);
				cout << mem.id <<":" << mem.port <<":" << mem.heartbeat << ":" << mem.timestamp << endl;
			}

			this->memberNode->memberList = memberList;

		}


		void MP1Node::logNodeRemove(Address *thisNode, Address *removedAddr) {
			static char stdstring[100];
			sprintf(stdstring, "Node %d.%d.%d.%d:%d removed at time %d", removedAddr->addr[0], removedAddr->addr[1], removedAddr->addr[2], removedAddr->addr[3], *(short *)&removedAddr->addr[4], par->getcurrtime());
		   // log->LOG(thisNode, stdstring);

		    log->LOG(thisNode,stdstring);

		}



