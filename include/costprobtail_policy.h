/*
 * ccnSim is a scalable chunk-level simulator for Content Centric
 * Networks (CCN), that we developed in the context of ANR Connect
 * (http://www.anr-connect.org/)
 *
 * People:
 *    Giuseppe Rossini (lead developer, mailto giuseppe.rossini@enst.fr)
 *    Raffaele Chiocchetti (developer, mailto raffaele.chiocchetti@gmail.com)
 *    Dario Rossi (occasional debugger, mailto dario.rossi@enst.fr)
 *    Andrea Araldo (mailto andrea.araldo@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef COSTPROBPRODTAIL_POLICY_H_
#define COSTPROBPRODTAIL_POLICY_H_

//<aa>
#include "decision_policy.h"
#include "error_handling.h"
#include "costprob_policy.h"
#include "lru_cache.h"
#include "WeightedContentDistribution.h"

class Costprobtail: public Costprob{
	protected:
		double alpha;
		lru_cache* mycache; // cache I'm attached to

    public:
		Costprobtail(double average_decision_ratio_, base_cache* mycache_par):
			Costprob(average_decision_ratio_)
		{

			if (xi>1 || xi<0){
				std::stringstream ermsg; 
				ermsg<<"xi="<<xi<<" is not valid";
				severe_error(__FILE__,__LINE__,ermsg.str().c_str() );
			}
			alpha = content_distribution_module->get_alpha();
			mycache = dynamic_cast<lru_cache*>(mycache_par);

			#ifdef SEVERE_DEBUG
			if( mycache == NULL ){
				std::stringstream ermsg; 
				ermsg<<"Costprobatailperf works only with lru";
				severe_error(__FILE__,__LINE__,ermsg.str().c_str() );
			}
			#endif

		};

		virtual bool data_to_cache(ccn_data * data_msg)
		{
			bool decision;

			#ifdef SEVERE_DEBUG
			if( !mycache->is_initialized() ){
				std::stringstream ermsg; 
				ermsg<<"base_cache is not initialized.";
				severe_error(__FILE__,__LINE__,ermsg.str().c_str() );
			}
			#endif

			if (! mycache->full() )
				decision = true;
			else{

				chunk_t content_index = data_msg->getChunk();
				double cost = data_msg->getCost();
				double new_content_weight = compute_content_weight(content_index,cost);
//				cout << "new_content_index="<<content_index<<"; popularity_estimation_new="<<
//					popularity_estimation<<"; cost of new="<<cost<<"; integral_cost_new="<<
//					integral_cost_new<<endl;


				lru_pos* lru_element_descriptor = mycache->get_lru();
				content_index = lru_element_descriptor->k;
				cost = lru_element_descriptor->cost;
				double lru_weight = compute_content_weight(content_index,cost);
//				cout << "lru_content_index="<<content_index<<"; popularity_estimation of lru="<<
//					popularity_estimation<<"; cost if lru="<<cost<<"; integral_cost_lru="<<
//					integral_cost_lru<<endl;

				if (new_content_weight > lru_weight)
					// Inserting this content in the cache would make it better
					decision = true;

				// a small xi means that we tend to renew the cache often

				else if ( dblrand() < xi )
					decision = false;
				else
					decision = true;
			}

			if (decision == true)
				set_last_accepted_content_cost(data_msg );

			return decision;
		};

		virtual double compute_correction_factor(){
			return 0;
		};

		virtual void after_insertion_action()
		{
			DecisionPolicy::after_insertion_action();
			#ifdef SEVERE_DEBUG
			if ( get_last_accepted_content_cost() == UNSET_COST ){
				std::stringstream ermsg; 
				ermsg<<"cost_of_the_last_accepted_element="<<get_last_accepted_content_cost() <<
					", while it MUST NOT be a negative number. Something goes wrong with the "<<
					"initialization of this attribute";
				severe_error(__FILE__,__LINE__,ermsg.str().c_str() );

			}
			#endif

			// Annotate the cost of the last inserted element
			mycache->get_mru()->cost = get_last_accepted_content_cost();

			#ifdef SEVERE_DEBUG
			// Unset this field to check if it is set again at the appropriate time
			// without erroneously use an old value
			last_accepted_content_cost = UNSET_COST;
			#endif
			
		}

		virtual double compute_content_weight(chunk_t id, double cost)=0;
};
//<//aa>
#endif

