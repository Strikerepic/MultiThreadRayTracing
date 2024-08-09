#ifndef THREAD_CORDINATOR_H
#define THREAD_CORDINATOR_H


struct tracker {
    tracker* newer;
    tracker* older;
};

class Thread_Cordinator {

    public:
        Thread_Cordinator() : numActive(0) {}

        tracker* generateNewKey() {
            if(numActive == 0) {
                tracker* a = new tracker();
                a->newer = nullptr;
                a->older = nullptr;
                
                oldestNode = a;
                youngestNode = a;
                numActive++;
                return a;
            }

            tracker* b = new tracker();
            b->newer = nullptr;
            b->older = youngestNode;

            youngestNode->newer = b;
            youngestNode = b;
            numActive++;
            return b;
        }

        bool ClearToPrintAndDestroy(tracker* keyToCheck) {
            if(keyToCheck->older == nullptr) {
                oldestNode = oldestNode->newer;
                if(oldestNode != nullptr) {
                    oldestNode->older = nullptr;
                }

                delete keyToCheck;
                numActive--;
                return true;
            }

            return false;
        }

        int numActiveThreads() {
            return numActive;
        }



    private:


    tracker* oldestNode;
    tracker* youngestNode;
    int numActive;

};


#endif