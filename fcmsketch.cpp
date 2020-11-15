#include "./src/FCMSketch/FCMSketch.h"

int main()
{
  if ((FCMSK_WL3 * FCMSK_K_ARY != FCMSK_WL2) or (FCMSK_WL2 * FCMSK_K_ARY != FCMSK_WL1))
    throw std::invalid_argument( "FCM-Sketch has inconsistent counter number for k-ary tree." );

	printf("Start accuracy measurement of FCM-Sketch\n");
	uint32_t totnum_packet = ReadTraces();

	FCMSketch * fcm = NULL;

	for(int iter_data = 0; iter_data < NUM_TRACE; ++iter_data){
		printf("[INFO] %3d-th trace starts to be processed..\n", iter_data+1);
		unordered_map<uint32_t, uint32_t> true_freq;
		vector<int> true_dist(1);
		fcm = new FCMSketch();
		int num_pkt = (int)traces[iter_data].size();

		for (int i = 0; i < num_pkt; ++i){
            true_freq[*((uint32_t*)(traces[iter_data][i].key))]++;
			fcm->insert((uint8_t*)(traces[iter_data][i].key), 1);
		}
        printf("[INFO] End of insertion process...Num.flow:%d\n",(int)true_freq.size());
		/*-*-*-* End of packet insertion *-*-*-*/

		double ARE = 0;
		double AAE = 0;
		set<uint32_t> HH_true;
		for(unordered_map<uint32_t, uint32_t>::iterator it = true_freq.begin(); it != true_freq.end(); ++it){
			uint8_t key[4] = {0}; // srcIP-flowkey
			uint32_t temp_first = htonl(it->first); // convert uint32_t -> uint8_t * 4 array
	        for (int i=0; i<4 ;++i){
            	key[i] = ((uint8_t*)&temp_first)[3-i];
	        }
	        if (it->second >= true_dist.size())
	        	true_dist.resize(it->second + 1, 0);
	        true_dist[it->second] += 1;
	        uint32_t est_val = fcm->query(key);
	        int dist = std::abs((int)it->second - (int)est_val);
	        ARE += dist * 1.0 / (it->second);
	        AAE += dist * 1.0;
	        if (it->second > HH_THRESHOLD)
	        	HH_true.insert(it->first);
		}
		ARE /= (int)true_freq.size();
		AAE /= (int)true_freq.size();

		printf("ARE : %.8lf, AAE : %.8lf\n", ARE, AAE);

		/*-*-*-* End of count-query (ARE, AAE) *-*-*-*/

		double HH_precision = 0;
        int HH_PR = 0;
        int HH_PR_denom = 0;
        int HH_RR = 0;
        int HH_RR_denom = 0;
        set<uint32_t>::iterator itr;
        for (itr = HH_true.begin(); itr != HH_true.end(); ++itr)
        {
            HH_PR_denom += 1;
            HH_PR += fcm->hh_candidates.find(*itr) != fcm->hh_candidates.end();
        }
        for (itr = fcm->hh_candidates.begin(); itr != fcm->hh_candidates.end(); ++itr)
        {
            HH_RR_denom += 1;
            HH_RR += HH_true.find(*itr) != HH_true.end();
        }
        HH_precision = (2 * (double(HH_PR) / double(HH_PR_denom)) * (double(HH_RR) / double(HH_RR_denom))) / ((double(HH_PR) / double(HH_PR_denom)) + (double(HH_RR) / double(HH_RR_denom)));
        printf("HH_precision : %3.5f\n", HH_precision);
        printf("HH_PR : %d, HH_PR_denom : %d, HH_RR : %d, HH_RR_denom : %d\n", HH_PR, HH_PR_denom, HH_RR, HH_RR_denom);



		/*-*-*-* End of HH detection (F1-score) *-*-*-*/


		int card = fcm->get_cardinality();
		double card_err = abs(card - int(true_freq.size())) / double(true_freq.size());
		printf("RE of cardinality : %2.6f (est=%8.0d, true=%8.0d)\n", card_err, card, (int)true_freq.size());


		/*-*-*-* End of cardinality estimation *-*-*-*/
        // WMRD initialization
        double WMRD = 0;
        double WMRD_nom = 0;
        double WMRD_denom = 0;


        // compute wmrd
	    vector<double> dist;		
        fcm->get_distribution(dist);

        for (int i = 1; i < true_dist.size(); ++i)
        {
            if (true_dist[i] == 0)
                continue;
            WMRD_nom += std::abs(true_dist[i] - dist[i]);
            WMRD_denom += double(true_dist[i] + dist[i])/2;
        }
        WMRD = WMRD_nom / WMRD_denom;
        printf("WMRD : %3.5f\n", WMRD);



        // entropy initialization
        double entropy_err = 0;
        double entropy_est = 0;

        double tot_est = 0;
        double entr_est = 0;

        for (int i = 1; i < dist.size(); ++i)
        {
            if (dist[i] == 0)
                continue;
            tot_est += i * dist[i];
            entr_est += i * dist[i] * log2(i);
        }
        entropy_est = - entr_est / tot_est + log2(tot_est);

        double entropy_true = 0;
        double tot_true = 0;
        double entr_true = 0;
        for (int i = 0; i < true_dist.size(); ++i)
        {
            if (true_dist[i] == 0)
                continue;
            tot_true += i * true_dist[i];
            entr_true += i * true_dist[i] * log2(i);
        }
        entropy_true = - entr_true / tot_true + log2(tot_true);

        entropy_err = std::abs(entropy_est - entropy_true) / entropy_true;
        printf("Entropy Relative Error (RE) = %f (true : %f, est : %f)\n", entropy_err, entropy_true, entropy_est);
		/*-*-*-* End of flow size distribution and entropy *-*-*-*/

        delete fcm;
	}
	printf("END\n");
    printf("\n\n\n");
}