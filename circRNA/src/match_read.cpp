#include "match_read.h"

char keep_log1[500];
char keep_log2[500];
static int trig = 0;

void get_mate_name(char* fq1, char* fq2) {
	strcpy(fq2, fq1);
	int i = strlen(fq1) - 1;
	while (fq1[i--] != '.' and i >= 1);
	if (fq1[i] == '1')
		fq2[i] = '2';
	else if (fq1[i] == '2')
		fq2[i] = '1';
	else {
		fprintf(stderr, "PE FASTQ names are not in the correct format\n");
		exit(1);
	}
}

void sort_positions(const bwtint_t& sp, const bwtint_t& ep, const int& len, vector<bwtint_t>& forward_list, bwtint_t& flist_size, vector<bwtint_t>& backward_list, bwtint_t& blist_size) {
	int dir=0;
	bwtint_t tmp;
	bwtint_t f_ind = 0, b_ind = 0;
	forward_list.clear();
	backward_list.clear();
	for (bwtint_t i = sp; i <= ep; i++) {
		tmp = get_pos(&i, len, dir);
		if (dir == 1)
			forward_list[f_ind++] = tmp;
		else
			backward_list[b_ind++] = tmp;

		//char *chr_name;
		//int32_t chr_len;
		//uint32_t chr_beg;
		//uint32_t chr_end;
		//bwt_get_intv_info(tmp, tmp + 20, &chr_name, &chr_len, &chr_beg, &chr_end);
		//fprintf(stderr, "Chr: %s,\t%d\n", chr_name, dir);
		//fprintf(stderr, "%"PRIu64"\n", chr_beg);
	}
	
	forward_list[f_ind] = -1;
	backward_list[b_ind] = -1;

	flist_size = f_ind;
	blist_size = b_ind;

	sort(forward_list.begin(), forward_list.end() + f_ind);
		sort(backward_list.begin(), backward_list.end() + b_ind);
}

// assumption: target is not less than list[0]
// input interval: [, )
// return: i if target in [i-1, i)
// => 
// closest Greater than: returned index
// closest Less than or Equal: returned index - 1
bwtint_t binary_search(const vector<bwtint_t>& list, bwtint_t beg, bwtint_t end, const bwtint_t& target) {
	if (end - beg <= 1)
		return end;
	bwtint_t mid = (beg + end) / 2;
	if (target < list[mid])
		return binary_search(list, beg, mid, target);
	else 
		return binary_search(list, mid, end, target);
}

bool is_concordant_sorted(const bwtint_t& sp_f, const bwtint_t& ep_f, const int& len_f, const bwtint_t& sp_b, const bwtint_t& ep_b, const int& len_b, const int& noise_thresh) {
	//if ((ep_f - sp_f >= REGIONSIZELIM) or (ep_b - sp_b >= REGIONSIZELIM))
	if ((ep_b - sp_b + 1) * (ep_f - sp_f + 1) >= RANGELIM)
		return false;

	//fprintf(stderr, "back size: %llu\nfront size: %llu\n", ep_b - sp_b + 1, ep_f - sp_f + 1);

	int dir_front=0, dir_back=0;
	bwtint_t flist_size_f, blist_size_f;
	bwtint_t flist_size_b, blist_size_b;

	bwtint_t spos_front, spos_back;
	vector<bwtint_t> forwardlist_f(ep_f - sp_f + 2);
	vector<bwtint_t> backwardlist_f(ep_f - sp_f + 2);
	vector<bwtint_t> forwardlist_b(ep_b - sp_b + 2);
	vector<bwtint_t> backwardlist_b(ep_b - sp_b + 2);

	sort_positions(sp_f, ep_f, len_f, forwardlist_f, flist_size_f, backwardlist_f, blist_size_f);
	sort_positions(sp_b, ep_b, len_b, forwardlist_b, flist_size_b, backwardlist_b, blist_size_b);
	
	//fprintf(stderr, "forward front size: %llu\nbackward front size: %llu\n", flist_size_f, blist_size_f);
	//fprintf(stderr, "forward back size: %llu\nbackward back size: %llu\n", flist_size_b, blist_size_b);

	//for (int k = 0; k < flist_size_f; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_f; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < flist_size_b; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_b[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_b; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_b[k]);
	//fprintf(stderr, "\n");

	bwtint_t i, j;
	if (flist_size_f > 0 and blist_size_b > 0) {
		if (flist_size_f <= blist_size_b) {
			for (i = 0; i < flist_size_f; i++) {
				if (forwardlist_f[i] < backwardlist_b[0])	// no position less than forwardlist_f[i] available on backwardlist_b
					continue;
				else {
					j = binary_search(backwardlist_b, 0, blist_size_b, forwardlist_f[i]);
				}
				spos_front = forwardlist_f[i];
				spos_back = backwardlist_b[j-1];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				//fprintf(stderr, "HEYYYYY %d\n%llu\n%llu\n", len_f, spos_front, spos_back + len_b + noise_thresh);
				if (spos_front <= spos_back + len_b + GENETHRESH)
					return true;
			}
		}
		else {
			for (j = 0; j < blist_size_b; j++) {
				if (backwardlist_b[j] < forwardlist_f[0])
					i = 0;
				else {
					i = binary_search(forwardlist_f, 0, flist_size_f, backwardlist_b[j]);
					if (i >= flist_size_f)	// no position greater than backwardlist_b[j] available on forwardlist_f
						continue;
				}
				spos_front = forwardlist_f[i];
				spos_back = backwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				if (spos_front <= spos_back + len_b + GENETHRESH)
					return true;
			}
		}
	}

	if (blist_size_f > 0 and flist_size_b > 0) {
		if (blist_size_f <= flist_size_b) {
			for (i = 0; i < blist_size_f; i++) {
				if (backwardlist_f[i] < forwardlist_b[0])
					j = 0;
				else {
					j = binary_search(forwardlist_b, 0, flist_size_b, backwardlist_f[i]);
					if (j >= flist_size_b)	// no position greater than backwardlist_f[i] available on backwardlist_f
						continue;
				}
				spos_front = backwardlist_f[i];
				spos_back = forwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				if (spos_back <= spos_front + len_f + GENETHRESH)
					return true;
			}
		}
		else {
			for (j = 0; j < flist_size_b; j++) {
				if (forwardlist_b[j] < backwardlist_f[0])	// no position less than forwardlist_b[j] available on backwardlist_f
					continue;
				else {
					i = binary_search(backwardlist_f, 0, blist_size_f, forwardlist_b[j]);
				}
				spos_front = backwardlist_f[i-1];
				spos_back = forwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				if (spos_back <= spos_front + len_f + GENETHRESH)
					return true;
			}
		}
	}
	return false;
}

bool is_concordant_sorted2(const bwtint_t& sp_f, const bwtint_t& ep_f, const int& len_f, const bwtint_t& sp_b, const bwtint_t& ep_b, const int& len_b, const int& noise_thresh, MatchedRead& mr) {
	//if ((ep_f - sp_f >= REGIONSIZELIM) or (ep_b - sp_b >= REGIONSIZELIM))
	if ((ep_b - sp_b + 1) * (ep_f - sp_f + 1) >= REGIONSIZELIM) {
		return false;
	}

	//fprintf(stderr, "back size: %llu\nfront size: %llu\n", ep_b - sp_b + 1, ep_f - sp_f + 1);

	int dir_front=0, dir_back=0;
	bwtint_t flist_size_f, blist_size_f;
	bwtint_t flist_size_b, blist_size_b;

	bwtint_t spos_front, spos_back;
	vector<bwtint_t> forwardlist_f(ep_f - sp_f + 2);
	vector<bwtint_t> backwardlist_f(ep_f - sp_f + 2);
	vector<bwtint_t> forwardlist_b(ep_b - sp_b + 2);
	vector<bwtint_t> backwardlist_b(ep_b - sp_b + 2);

	sort_positions(sp_f, ep_f, len_f, forwardlist_f, flist_size_f, backwardlist_f, blist_size_f);
	sort_positions(sp_b, ep_b, len_b, forwardlist_b, flist_size_b, backwardlist_b, blist_size_b);
	
	//fprintf(stderr, "forward front size: %llu\nbackward front size: %llu\n", flist_size_f, blist_size_f);
	//fprintf(stderr, "forward back size: %llu\nbackward back size: %llu\n", flist_size_b, blist_size_b);

	//for (int k = 0; k < flist_size_f; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_f; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < flist_size_b; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_b[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_b; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_b[k]);
	//fprintf(stderr, "\n");

	bwtint_t i, j;
	if (flist_size_f > 0 and blist_size_b > 0) {
		if (flist_size_f <= blist_size_b) {
			for (i = 0; i < flist_size_f; i++) {
				if (forwardlist_f[i] < backwardlist_b[0])	// no position less than forwardlist_f[i] available on backwardlist_b
					continue;
				else {
					j = binary_search(backwardlist_b, 0, blist_size_b, forwardlist_f[i]);
				}
				spos_front = forwardlist_f[i];
				spos_back = backwardlist_b[j-1];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				//fprintf(stderr, "HEYYYYY %d\n%llu\n%llu\n", len_f, spos_front, spos_back + len_b + noise_thresh);
				if (spos_front <= spos_back + len_b + GENETHRESH) {
					mr.is_concord = true;
					mr.start_pos = spos_back;
					mr.matched_len = spos_front + len_f - spos_back + 1;
					mr.dir = -1;
					return true;
				}
			}
		}
		else {
			for (j = 0; j < blist_size_b; j++) {
				if (backwardlist_b[j] < forwardlist_f[0])
					i = 0;
				else {
					i = binary_search(forwardlist_f, 0, flist_size_f, backwardlist_b[j]);
					if (i >= flist_size_f)	// no position greater than backwardlist_b[j] available on forwardlist_f
						continue;
				}
				spos_front = forwardlist_f[i];
				spos_back = backwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				if (spos_front <= spos_back + len_b + GENETHRESH) {
					mr.is_concord = true;
					mr.start_pos = spos_back;
					mr.matched_len = spos_front + len_f - spos_back + 1;
					mr.dir = -1;
					return true;
				}
			}
		}
	}

	if (blist_size_f > 0 and flist_size_b > 0) {
		if (blist_size_f <= flist_size_b) {
			for (i = 0; i < blist_size_f; i++) {
				if (backwardlist_f[i] < forwardlist_b[0])
					j = 0;
				else {
					j = binary_search(forwardlist_b, 0, flist_size_b, backwardlist_f[i]);
					if (j >= flist_size_b)	// no position greater than backwardlist_f[i] available on backwardlist_f
						continue;
				}
				spos_front = backwardlist_f[i];
				spos_back = forwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				if (spos_back <= spos_front + len_f + GENETHRESH) {
					mr.is_concord = true;
					mr.start_pos = spos_front;
					mr.matched_len = spos_back + len_b - spos_front + 1;
					mr.dir = 1;
					return true;
				}
			}
		}
		else {
			for (j = 0; j < flist_size_b; j++) {
				if (forwardlist_b[j] < backwardlist_f[0])	// no position less than forwardlist_b[j] available on backwardlist_f
					continue;
				else {
					i = binary_search(backwardlist_f, 0, blist_size_f, forwardlist_b[j]);
				}
				spos_front = backwardlist_f[i-1];
				spos_back = forwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				if (spos_back <= spos_front + len_f + GENETHRESH) {
					mr.is_concord = true;
					mr.start_pos = spos_front;
					mr.matched_len = spos_back + len_b - spos_front + 1;
					mr.dir = 1;
					return true;
				}
			}
		}
	}
	return false;
}

bool is_concordant_sorted_same_strand(const bwtint_t& sp_f, const bwtint_t& ep_f, const int& len_f, const bwtint_t& sp_b, const bwtint_t& ep_b, const int& len_b, const int& noise_thresh, MatchedRead& mr) {
	//if ((ep_f - sp_f >= REGIONSIZELIM) or (ep_b - sp_b >= REGIONSIZELIM))
	//if ((ep_b - sp_b + 1) * (ep_f - sp_f + 1) >= REGIONSIZELIM) {
	if ((ep_b - sp_b + 1) * (ep_f - sp_f + 1) >= RANGELIM) {
		return false;
	}

	//fprintf(stderr, "back size: %llu\nfront size: %llu\n", ep_b - sp_b + 1, ep_f - sp_f + 1);

	int dir_front=0, dir_back=0;
	bwtint_t flist_size_f, blist_size_f;
	bwtint_t flist_size_b, blist_size_b;

	bwtint_t spos_front, spos_back;
	vector<bwtint_t> forwardlist_f(ep_f - sp_f + 2);
	vector<bwtint_t> backwardlist_f(ep_f - sp_f + 2);
	vector<bwtint_t> forwardlist_b(ep_b - sp_b + 2);
	vector<bwtint_t> backwardlist_b(ep_b - sp_b + 2);

	sort_positions(sp_f, ep_f, len_f, forwardlist_f, flist_size_f, backwardlist_f, blist_size_f);
	sort_positions(sp_b, ep_b, len_b, forwardlist_b, flist_size_b, backwardlist_b, blist_size_b);
	
	//fprintf(stderr, "forward front size: %llu\nbackward front size: %llu\n", flist_size_f, blist_size_f);
	//fprintf(stderr, "forward back size: %llu\nbackward back size: %llu\n", flist_size_b, blist_size_b);

	//for (int k = 0; k < flist_size_f; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_f; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < flist_size_b; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_b[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_b; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_b[k]);
	//fprintf(stderr, "\n");

	bwtint_t i, j;
	if (flist_size_f > 0 and flist_size_b > 0) {
		if (flist_size_f <= flist_size_b) {
			for (i = 0; i < flist_size_f; i++) {
				if (forwardlist_f[i] < forwardlist_b[0])	// no position less than forwardlist_f[i] available on backwardlist_b
					j = 0;
				else {
					j = binary_search(forwardlist_b, 0, flist_size_b, forwardlist_f[i]);
					if (j >= flist_size_b)
						continue;
				}
				spos_front = forwardlist_f[i];
				spos_back = forwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				//fprintf(stderr, "HEYYYYY %d\n%llu\n%llu\n", len_f, spos_front, spos_back + len_b + noise_thresh);
				if (spos_back <= spos_front + len_f + GENETHRESH) {
					mr.is_concord = true;
					mr.start_pos = spos_front;
					mr.matched_len = spos_back + len_b - spos_front + 1;
					mr.dir = 1;
					return true;
				}
			}
		}
		else {
			for (j = 0; j < flist_size_b; j++) {
				if (forwardlist_b[j] < forwardlist_f[0])
					continue;
				else {
					i = binary_search(forwardlist_f, 0, flist_size_f, forwardlist_b[j]);
				}
				spos_front = forwardlist_f[i-1];
				spos_back = forwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				if (spos_back <= spos_front + len_f + GENETHRESH) {
					mr.is_concord = true;
					mr.start_pos = spos_front;
					mr.matched_len = spos_back + len_b - spos_front + 1;
					mr.dir = 1;
					return true;
				}
			}
		}
	}

	if (blist_size_f > 0 and blist_size_b > 0) {
		if (blist_size_f <= blist_size_b) {
			for (i = 0; i < blist_size_f; i++) {
				if (backwardlist_f[i] < backwardlist_b[0])
					continue;
				else {
					j = binary_search(backwardlist_b, 0, blist_size_b, backwardlist_f[i]);
				}
				spos_front = backwardlist_f[i];
				spos_back = backwardlist_b[j-1];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				if (spos_front <= spos_back + len_b + GENETHRESH) {
					mr.is_concord = true;
					mr.start_pos = spos_back;
					mr.matched_len = spos_front + len_f - spos_back + 1;
					mr.dir = -1;
					return true;
				}
			}
		}
		else {
			for (j = 0; j < blist_size_b; j++) {
				if (backwardlist_b[j] < backwardlist_f[0])	// no position less than forwardlist_b[j] available on backwardlist_f
					i = 0;
				else {
					i = binary_search(backwardlist_f, 0, blist_size_f, backwardlist_b[j]);
					if (i >= blist_size_f)	// no position greater than backwardlist_f[i] available on backwardlist_f
						continue;
				}
				spos_front = backwardlist_f[i];
				spos_back = backwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				if (spos_front <= spos_back + len_b + GENETHRESH) {
					mr.is_concord = true;
					mr.start_pos = spos_back;
					mr.matched_len = spos_front + len_f - spos_back + 1;
					mr.dir = -1;
					return true;
				}
			}
		}
	}
	return false;
}

bool is_concordant(const bwtint_t& sp_f, const bwtint_t& ep_f, const int& len_f, const bwtint_t& sp_b, const bwtint_t& ep_b, const int& len_b, const int& noise_thresh) {
	//fprintf(stderr, "front: %llu\t%llu\n", sp_f, ep_f);
	//fprintf(stderr, "back : %llu\t%llu\n", sp_b, ep_b);
	
	if ((ep_b - sp_b + 1) * (ep_f - sp_f + 1) >= RANGELIM)
		return false;

	//fprintf(stderr, "back size: %llu\nfront size: %llu\n", ep_b - sp_b + 1, ep_f - sp_f + 1);

	int dir_front, dir_back;
	bwtint_t spos_front, spos_back;
	for (bwtint_t i = sp_f; i <= ep_f; i++) {
		for (bwtint_t j = sp_b; j <= ep_b; j++) {
			spos_front = get_pos(&i, len_f, dir_front);
			spos_back = get_pos(&j, len_b, dir_back);
			//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
			
			//char *chr_name;
			//int32_t chr_len;
			//uint32_t chr_beg;
			//uint32_t chr_end;
			//bwt_get_intv_info(spos_front, spos_front + 20, &chr_name, &chr_len, &chr_beg, &chr_end);
			//fprintf(stderr, "Chr: %s\n", chr_name);
			//fprintf(stderr, "%"PRIu64"\n", chr_beg);
			//bwt_get_intv_info(spos_back, spos_back + 20, &chr_name, &chr_len, &chr_beg, &chr_end);
			//fprintf(stderr, "%"PRIu64"\n", chr_beg);

			if (dir_back == 1) {
				if ((dir_front == -1) and (spos_back - spos_front - len_f <= GENETHRESH))
					return true;
			}
			else {
				if ((dir_front == 1) and (spos_front - spos_back - len_b <= GENETHRESH))
					return true;
			}
		}
	}
	return false;
}

// --return true if potentially chimeric
// 0 : concordant
// 1 : potentially chimeric (keep)
int find_expanded_positions(const char* rseq, const char* rcseq, const int& rseq_len) {
	bwtint_t sp_b, ep_b;
	bwtint_t sp_f, ep_f;
	int exp_len_back = 0;
	int exp_len_front = 0;

	const int noise_thresh = rseq_len / 10;

	exp_len_front = get_expanded_locs(rcseq, rseq_len, &sp_f, &ep_f);
	if (exp_len_front >= rseq_len - noise_thresh) {	// concordant
		fprintf(stderr, "[Concordant-e]\tfront matched: %d", exp_len_front);
		return 0;
	}

	exp_len_back = get_expanded_locs(rseq + exp_len_front, rseq_len - exp_len_front, &sp_b, &ep_b);
	//exp_len_back = get_expanded_locs(rseq + exp_len_front, rseq_len - exp_len_front - noise_thresh, &sp_b, &ep_b);
	fprintf(stderr, "%s\tlen front: %d,\tlen back: %d\n", rseq, exp_len_front, exp_len_back);
	//if (occ > 0)
	//	locate_match(&sp, &ep, window_size);
	
	if (exp_len_front + exp_len_back <= rseq_len / 2) {
		fprintf(stderr, "[Chimeric-e]\tfront matched: %d, back matched: %d", exp_len_front, exp_len_back);
		return 1;
	}
	
	//bool consis = check_cosistent_match(&sp_b, &ep_b, exp_len_backward, &sp_f, &ep_f, exp_len_forward);
	bool consis = is_concordant(sp_f, ep_f, exp_len_front, sp_b, ep_b, exp_len_back, noise_thresh);
	//bool consis = is_concordant_sorted(sp_f, ep_f, exp_len_front, sp_b, ep_b, exp_len_back, noise_thresh);

	std::string res = (consis) ? "Concordant" : "Chimeric";
	fprintf(stderr, "[%s]\tfront matched: %d, back matched: %d", res.c_str(), exp_len_front, exp_len_back);

	if (consis)
		return 0;
	else
		return 1;
}

// --return true if potentially chimeric
// 0 : concordant
// 1 : potentially chimeric (keep)
int find_expanded_positions2(const char* rseq, const char* rcseq, const int& rseq_len, MatchedRead& mr) {
	bwtint_t sp_b, ep_b;
	bwtint_t sp_f, ep_f;
	int exp_len_back = 0;
	int exp_len_front = 0;

	const int noise_thresh = rseq_len / 10;

	mr.is_concord = false;

	exp_len_front = get_expanded_locs(rcseq, rseq_len, &sp_f, &ep_f);
	if (exp_len_front >= rseq_len - noise_thresh) {	// concordant
		fprintf(stderr, "[Concordant-e]\tfront matched: %d\n", exp_len_front);
		mr.is_concord = true;
		mr.start_pos = get_pos(&sp_f, exp_len_front, mr.dir);
		mr.dir *= -1;
		mr.matched_len = exp_len_front;
		return 0;
	}

	exp_len_back = get_expanded_locs(rseq + exp_len_front, rseq_len - exp_len_front, &sp_b, &ep_b);
	fprintf(stderr, "%s\tlen front: %d,\tlen back: %d\n", rseq, exp_len_front, exp_len_back);
	
	//if (exp_len_front < 23 or  exp_len_back < 23) {
	if (exp_len_front + exp_len_back <= 0.6 * rseq_len) {
		fprintf(stderr, "[Chimeric-e]\tfront matched: %d, back matched: %d\n", exp_len_front, exp_len_back);
		return 1;
	}
	
	//bool consis = check_cosistent_match(&sp_b, &ep_b, exp_len_backward, &sp_f, &ep_f, exp_len_forward);
	//bool consis = is_concordant(sp_f, ep_f, exp_len_front, sp_b, ep_b, exp_len_back, noise_thresh);
	bool consis = is_concordant_sorted2(sp_f, ep_f, exp_len_front, sp_b, ep_b, exp_len_back, noise_thresh, mr);

	std::string res = (consis) ? "Concordant" : "Chimeric";
	fprintf(stderr, "[%s]\tfront matched: %d, back matched: %d\n", res.c_str(), exp_len_front, exp_len_back);

	if (consis)
		return 0;
	else
		return 1;
}

int check_concordant_mates(const Record* m1, const Record* m2) {
	MatchedRead mr1, mr2;
	int chimeric1 = find_expanded_positions2(m1->seq, m1->rcseq, m1->seq_len, mr1);
	int chimeric2 = find_expanded_positions2(m2->seq, m2->rcseq, m2->seq_len, mr2);

	if (chimeric1 == 1 and chimeric2 == 1)
		return 1;

	if (chimeric1 == 1 or chimeric2 == 1) {
		//fprintf(stderr, "Half concordant: %s", m1->rname);
		return 2;
	}

	//fprintf(stderr, "%s", m1->rname);
	//fprintf(stderr, "M1: dir:%d,\tstart:%llu,\tend:%llu\n", mr1.dir, mr1.start_pos, mr1.start_pos + mr1.matched_len);
	//fprintf(stderr, "M2: dir:%d,\tstart:%llu,\tend:%llu\n", mr2.dir, mr2.start_pos, mr2.start_pos + mr2.matched_len);

	if (mr1.dir == 1 and mr2.dir == -1 and (mr1.start_pos <= mr2.start_pos))
		return 0;
	if (mr1.dir == -1 and mr2.dir == 1 and (mr2.start_pos <= mr1.start_pos))
		return 0;
	return 3;
}

int find_exact_positions(const char* rseq, int rseq_len, int window_size) {
	bwtint_t sp, ep;
	int occ = 0;

	for (int i = 0; i < rseq_len; i += window_size) {
		occ = get_exact_locs(rseq + i, window_size, &sp, &ep);
		//fprintf(stderr, "Start pos: %d\n", i);
		//fprintf(stderr, "Number of matches: %d\n", occ);

		if (occ > 0)
			locate_match(&sp, &ep, window_size);
	}

	return occ;
}

int find_exact_positions_slide(const char* rseq, int rseq_len, const int& window_size, const int& shift_step, MatchedRead& mr) {
	bwtint_t sp_f, ep_f;
	bwtint_t sp_b, ep_b;
	int occ = 0, i, j;
	MatchedRead mr1, mr2;

	mr1.is_concord = false;
	for (i = 0; i <= rseq_len - 2*window_size; i += shift_step) {
		occ = get_exact_locs(rseq + i, window_size, &sp_f, &ep_f);
		fprintf(stderr, "Start pos: %d\n", i);
		fprintf(stderr, "Number of matches: %d\n", occ);

		if (occ > 0) {
			mr1.is_concord = true;
			mr1.matched_len = window_size;
			break;
		}
	}

	mr2.is_concord = false;
	for (j = rseq_len - window_size; j >= i + window_size; j -= shift_step) {
		occ = get_exact_locs(rseq + j, window_size, &sp_b, &ep_b);
		fprintf(stderr, "Start pos: %d\n", j);
		fprintf(stderr, "Number of matches: %d\n", occ);

		if (occ > 0) {
			mr2.is_concord = true;
			mr2.matched_len = window_size;
			break;
		}
	}

	mr.is_concord = mr1.is_concord && mr2.is_concord;
	if (!mr.is_concord) {
		return 1;
	}
	
	bool consis = is_concordant_sorted_same_strand(sp_f, ep_f, mr1.matched_len, sp_b, ep_b, mr2.matched_len, rseq_len/10, mr);

	std::string res = (consis) ? "Concordant" : "Chimeric";
	fprintf(stderr, "[%s]\tfront matched: %d, back matched: %d\n", res.c_str(), mr1.matched_len, mr2.matched_len);
	fprintf(stderr, "front start rpos: %d, back start rpos: %d\n", i, j);

	if (consis)
		return 0;
	else 
		return 1;
}

// return value:
// 0: concordant
// 1: potentially chimeric
// 2: definitely chimeric
int check_concordant_mates_noexpand(const Record* m1, const Record* m2) {
	MatchedRead mr1, mr2;
	int chimeric1 = find_exact_positions_slide(m1->seq, m1->seq_len, 28, 3, mr1);
	int chimeric2 = find_exact_positions_slide(m2->seq, m2->seq_len, 28, 3, mr2);

	//fprintf(stderr, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

	if (chimeric1 == 1 and chimeric2 == 1)
		return 1;

	if (chimeric1 == 1 or chimeric2 == 1) {
		//fprintf(stderr, "Half concordant: %s", m1->rname);
		return 1;
	}

	//fprintf(stderr, "%s", m1->rname);
	//fprintf(stderr, "M1: dir:%d,\tstart:%llu,\tend:%llu\n", mr1.dir, mr1.start_pos, mr1.start_pos + mr1.matched_len);
	//fprintf(stderr, "M2: dir:%d,\tstart:%llu,\tend:%llu\n", mr2.dir, mr2.start_pos, mr2.start_pos + mr2.matched_len);

	if (mr1.dir == 1 and mr2.dir == -1 and (mr1.start_pos <= mr2.start_pos))
		return 0;
	if (mr1.dir == -1 and mr2.dir == 1 and (mr2.start_pos <= mr1.start_pos))
		return 0;

	if (mr1.dir == 1 and mr2.dir == -1 and (mr1.start_pos > mr2.start_pos + 8) and (mr1.start_pos <= mr2.start_pos + 1000)) {
		fprintf(stderr, "%s", m1->rname);
		fprintf(stderr, "M1: dir: %d\t%llu\n", mr1.dir, mr1.start_pos);
		fprintf(stderr, "M2: dir: %d\t%llu\n", mr2.dir, mr2.start_pos);

		char *chr_name;
		int32_t chr_len;
		uint32_t chr_beg;
		uint32_t chr_end;
		bwt_get_intv_info(mr1.start_pos, mr1.start_pos + 20, &chr_name, &chr_len, &chr_beg, &chr_end);
		fprintf(stderr, "Chr: %s\t", chr_name);
		fprintf(stderr, "%"PRIu64"\n", chr_beg);
		bwt_get_intv_info(mr2.start_pos, mr2.start_pos + 20, &chr_name, &chr_len, &chr_beg, &chr_end);
		fprintf(stderr, "Chr: %s\t", chr_name);
		fprintf(stderr, "%"PRIu64"\n", chr_beg);

		return 2;
	}
	if (mr1.dir == -1 and mr2.dir == 1 and (mr2.start_pos > mr1.start_pos + 8) and (mr2.start_pos <= mr1.start_pos + 1000)) {
		fprintf(stderr, "%s", m1->rname);
		fprintf(stderr, "M1: dir: %d\t%llu\n", mr1.dir, mr1.start_pos);
		fprintf(stderr, "M2: dir: %d\t%llu\n", mr2.dir, mr2.start_pos);

		char *chr_name;
		int32_t chr_len;
		uint32_t chr_beg;
		uint32_t chr_end;
		bwt_get_intv_info(mr1.start_pos, mr1.start_pos + 20, &chr_name, &chr_len, &chr_beg, &chr_end);
		fprintf(stderr, "Chr: %s\t", chr_name);
		fprintf(stderr, "%"PRIu64"\n", chr_beg);
		bwt_get_intv_info(mr2.start_pos, mr2.start_pos + 20, &chr_name, &chr_len, &chr_beg, &chr_end);
		fprintf(stderr, "Chr: %s\t", chr_name);
		fprintf(stderr, "%"PRIu64"\n", chr_beg);

		return 2;
	}
	return 1;
}

bool is_chimeric_intersect(const vector<bwtint_t>& forwardlist_f, const bwtint_t& flist_size_f, const vector<bwtint_t>& backwardlist_f, const bwtint_t& blist_size_f, const int& len_f, 
							const vector<bwtint_t>& forwardlist_b, const bwtint_t& flist_size_b, const vector<bwtint_t>& backwardlist_b, const bwtint_t& blist_size_b, const int& len_b) {
	//if ((ep_f - sp_f >= REGIONSIZELIM) or (ep_b - sp_b >= REGIONSIZELIM))
	if (flist_size_f >= REGIONSIZELIM or blist_size_f >= REGIONSIZELIM or flist_size_b >= REGIONSIZELIM or blist_size_b >= REGIONSIZELIM) {
		return false;
	}

	char *chr_name_f, *chr_name_b;
	int32_t chr_len_f, chr_len_b;
	uint32_t chr_beg_f, chr_beg_b;
	uint32_t chr_end_f, chr_end_b;

	int dir_front=0, dir_back=0;
	bwtint_t spos_front, spos_back;
	
	//fprintf(stderr, "forward front size: %llu\nbackward front size: %llu\n", flist_size_f, blist_size_f);
	//fprintf(stderr, "forward back size: %llu\nbackward back size: %llu\n", flist_size_b, blist_size_b);

	//for (int k = 0; k < flist_size_f; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_f; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < flist_size_b; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_b[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_b; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_b[k]);
	//fprintf(stderr, "\n");

	bwtint_t i, j, target;

	// on reverse strand
	if (flist_size_f > 0 and blist_size_b > 0) {
		if (flist_size_f <= blist_size_b) {
			for (i = 0; i < flist_size_f; i++) {
				target = forwardlist_f[i] + len_f - 1;
				if (target < backwardlist_b[0])
					j = 0;
				else {
					j = binary_search(backwardlist_b, 0, blist_size_b, target);
					if (j >= blist_size_b)
						continue;
				}
				spos_front = forwardlist_f[i];
				spos_back = backwardlist_b[j];
				//fprintf(stderr, "111---Start pos front: %llu\n---Start pos back: %llu\n len front: %d\n len back: %d\n", spos_front, spos_back, len_f, len_b);
			
				bwt_get_intv_info(spos_front, spos_front + len_f - 1, &chr_name_f, &chr_len_f, &chr_beg_f, &chr_end_f);
				bwt_get_intv_info(spos_back,  spos_back  + len_b - 1, &chr_name_b, &chr_len_b, &chr_beg_b, &chr_end_b);
				
				if (strcmp(chr_name_f, chr_name_b) == 0 and spos_back + len_b <= spos_front + GENETHRESH) {
					fprintf(outputJuncFile, "%s\t%d\t%d\t-\t%s\t%d\t%d\t-\t", chr_name_b, chr_beg_b, chr_end_b, chr_name_f, chr_beg_f, chr_end_f);
					return true;
				}
			}
		}
		else {
			for (j = 0; j < blist_size_b; j++) {
				target = backwardlist_b[j] - len_f;
				if (target < forwardlist_f[0])
					continue;
				else {
					i = binary_search(forwardlist_f, 0, flist_size_f, target);
				}
				spos_front = forwardlist_f[i-1];
				spos_back = backwardlist_b[j];
				//fprintf(stderr, "222---Start pos front: %llu\n---Start pos back: %llu\n len front: %d\n len back: %d\n", spos_front, spos_back, len_f, len_b);

				bwt_get_intv_info(spos_front, spos_front + len_f - 1, &chr_name_f, &chr_len_f, &chr_beg_f, &chr_end_f);
				bwt_get_intv_info(spos_back,  spos_back  + len_b - 1, &chr_name_b, &chr_len_b, &chr_beg_b, &chr_end_b);
				
				if (strcmp(chr_name_f, chr_name_b) == 0 and spos_back + len_b <= spos_front + GENETHRESH) {
					fprintf(outputJuncFile, "%s\t%d\t%d\t-\t%s\t%d\t%d\t-\t", chr_name_b, chr_beg_b, chr_end_b, chr_name_f, chr_beg_f, chr_end_f);
					return true;
				}
			}
		}
	}

	// on forward strand
	if (blist_size_f > 0 and flist_size_b > 0) {
		if (blist_size_f <= flist_size_b) {
			for (i = 0; i < blist_size_f; i++) {
				target = backwardlist_f[i] - len_b;
				if (target < forwardlist_b[0])
					continue;
				else {
					j = binary_search(forwardlist_b, 0, flist_size_b, target);
				}
				spos_front = backwardlist_f[i];
				spos_back = forwardlist_b[j-1];
				//fprintf(stderr, "333---Start pos front: %llu\n---Start pos back: %llu\n len front: %d\n len back: %d\n", spos_front, spos_back, len_f, len_b);
				
				bwt_get_intv_info(spos_front, spos_front + len_f - 1, &chr_name_f, &chr_len_f, &chr_beg_f, &chr_end_f);
				bwt_get_intv_info(spos_back,  spos_back  + len_b - 1, &chr_name_b, &chr_len_b, &chr_beg_b, &chr_end_b);
				
				if (strcmp(chr_name_f, chr_name_b) == 0 and spos_front + len_f <= spos_back + GENETHRESH) {
					fprintf(outputJuncFile, "%s\t%d\t%d\t+\t%s\t%d\t%d\t+\t", chr_name_f, chr_beg_f, chr_end_f, chr_name_b, chr_beg_b, chr_end_b);
					return true;
				}
			}
		}
		else {
			for (j = 0; j < flist_size_b; j++) {
				target = forwardlist_b[j] + len_b - 1;
				if (target < backwardlist_f[0])	// no position less than forwardlist_b[j] available on backwardlist_f
					i = 0;
				else {
					i = binary_search(backwardlist_f, 0, blist_size_f, target);
					if (i >= blist_size_f)
						continue;
				}
				spos_front = backwardlist_f[i];
				spos_back = forwardlist_b[j];
				//fprintf(stderr, "444---Start pos front: %llu\n---Start pos back: %llu\n len front: %d\n len back: %d\n", spos_front, spos_back, len_f, len_b);
				
				bwt_get_intv_info(spos_front, spos_front + len_f - 1, &chr_name_f, &chr_len_f, &chr_beg_f, &chr_end_f);
				bwt_get_intv_info(spos_back,  spos_back  + len_b - 1, &chr_name_b, &chr_len_b, &chr_beg_b, &chr_end_b);
				
				if (strcmp(chr_name_f, chr_name_b) == 0 and spos_front + len_f <= spos_back + GENETHRESH) {
					fprintf(outputJuncFile, "%s\t%d\t%d\t+\t%s\t%d\t%d\t+\t", chr_name_f, chr_beg_f, chr_end_f, chr_name_b, chr_beg_b, chr_end_b);
					return true;
				}
			}
		}
	}
	return false;
}

// return value:
// 1: concordant juinction
// 2: chimeric
// 3: discordant
int intersect(const bwtint_t& sp_f, const bwtint_t& ep_f, const int& len_f, const bwtint_t& sp_b, const bwtint_t& ep_b, const int& len_b, vector <MatchedRead>& mrl, int& mrl_size, bool same_strand) {
	if ((ep_b - sp_b) >= REGIONSIZELIM or (ep_f - sp_f) >= REGIONSIZELIM) {
		mrl_size = 0;
		//if (trig == 0)
		//	sprintf(keep_log1, "LongList\t%llu\t%llu\t", ep_f - sp_f + 1, ep_b - sp_b + 1);
		//else
		//	sprintf(keep_log2, "LongList\t%llu\t%llu\t", ep_f - sp_f + 1, ep_b - sp_b + 1);
		return 3;
	}

	char *chr_name_f, *chr_name_b;
	int32_t chr_len_f, chr_len_b;
	uint32_t chr_beg_f, chr_beg_b;
	uint32_t chr_end_f, chr_end_b;

	int dir_front=0, dir_back=0;
	bwtint_t flist_size_f, blist_size_f;
	bwtint_t flist_size_b, blist_size_b;

	bwtint_t spos_front, spos_back;
	vector<bwtint_t> forwardlist_f(ep_f - sp_f + 2);
	vector<bwtint_t> backwardlist_f(ep_f - sp_f + 2);
	vector<bwtint_t> forwardlist_b(ep_b - sp_b + 2);
	vector<bwtint_t> backwardlist_b(ep_b - sp_b + 2);

	if (! same_strand) {
		sort_positions(sp_f, ep_f, len_f, forwardlist_f, flist_size_f, backwardlist_f, blist_size_f);
		sort_positions(sp_b, ep_b, len_b, forwardlist_b, flist_size_b, backwardlist_b, blist_size_b);
	}
	else {
		sort_positions(sp_f, ep_f, len_f, forwardlist_f, flist_size_f, backwardlist_f, blist_size_f);
		sort_positions(sp_b, ep_b, len_b, backwardlist_b, blist_size_b, forwardlist_b, flist_size_b);
	}
	
	//fprintf(stderr, "forward front size: %llu\nbackward front size: %llu\n", flist_size_f, blist_size_f);
	//fprintf(stderr, "forward back size: %llu\nbackward back size: %llu\n", flist_size_b, blist_size_b);

	//for (int k = 0; k < flist_size_f; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_f; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_f[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < flist_size_b; k++)
	//	fprintf(stderr, "%llu\t", forwardlist_b[k]);
	//fprintf(stderr, "\n");
	//for (int k = 0; k < blist_size_b; k++)
	//	fprintf(stderr, "%llu\t", backwardlist_b[k]);
	//fprintf(stderr, "\n");

	bwtint_t i, j, target;
	mrl_size = 0;
	
	// on reverse strand
	if (flist_size_f > 0 and blist_size_b > 0) {
		if (flist_size_f <= blist_size_b) {
			for (i = 0; i < flist_size_f; i++) {
				target = forwardlist_f[i] - len_b;
				if (target < backwardlist_b[0])	// no position less than or equal to target available on backwardlist_b
					continue;
				else {
					j = binary_search(backwardlist_b, 0, blist_size_b, target);
				}
				spos_front = forwardlist_f[i];
				spos_back = backwardlist_b[j-1];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				bwt_get_intv_info(spos_front, spos_front + len_f - 1, &chr_name_f, &chr_len_f, &chr_beg_f, &chr_end_f);
				bwt_get_intv_info(spos_back,  spos_back  + len_b - 1, &chr_name_b, &chr_len_b, &chr_beg_b, &chr_end_b);
			
				if (strcmp(chr_name_f, chr_name_b) == 0 and chr_beg_f + len_f <= chr_beg_b + GENETHRESH) {
					mrl[mrl_size].is_concord = true;
					mrl[mrl_size].chr = chr_name_f;
					mrl[mrl_size].start_pos = chr_beg_b;
					mrl[mrl_size].end_pos = chr_end_f;
					mrl[mrl_size].matched_len = spos_front + len_f - spos_back;
					mrl[mrl_size].dir = -1;
					if (++mrl_size >= MRLSIZELIM)
						return 1;
				}
			}
		}
		else {
			for (j = 0; j < blist_size_b; j++) {
				target = backwardlist_b[j] + len_b - 1;
				if (target < forwardlist_f[0])
					i = 0;
				else {
					i = binary_search(forwardlist_f, 0, flist_size_f, target);
					if (i >= flist_size_f)	// no position greater than backwardlist_b[j] available on forwardlist_f
						continue;
				}
				spos_front = forwardlist_f[i];
				spos_back = backwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				bwt_get_intv_info(spos_front, spos_front + len_f - 1, &chr_name_f, &chr_len_f, &chr_beg_f, &chr_end_f);
				bwt_get_intv_info(spos_back,  spos_back  + len_b - 1, &chr_name_b, &chr_len_b, &chr_beg_b, &chr_end_b);
			
				if (strcmp(chr_name_f, chr_name_b) == 0 and chr_beg_f + len_f <= chr_beg_b + GENETHRESH) {
					mrl[mrl_size].is_concord = true;
					mrl[mrl_size].chr = chr_name_f;
					mrl[mrl_size].start_pos = chr_beg_b;
					mrl[mrl_size].end_pos = chr_end_f;
					mrl[mrl_size].matched_len = spos_front + len_f - spos_back + 1;
					mrl[mrl_size].dir = -1;
					if (++mrl_size >= MRLSIZELIM)
						return 1;
				}
			}
		}
	}

	// on forward strand
	if (blist_size_f > 0 and flist_size_b > 0) {
		if (blist_size_f <= flist_size_b) {
			for (i = 0; i < blist_size_f; i++) {
				target = backwardlist_f[i] + len_f - 1;
				if (target < forwardlist_b[0])
					j = 0;
				else {
					j = binary_search(forwardlist_b, 0, flist_size_b, target);
					if (j >= flist_size_b)	// no position greater than backwardlist_f[i] available on backwardlist_f
						continue;
				}
				spos_front = backwardlist_f[i];
				spos_back = forwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				bwt_get_intv_info(spos_front, spos_front + len_f - 1, &chr_name_f, &chr_len_f, &chr_beg_f, &chr_end_f);
				bwt_get_intv_info(spos_back,  spos_back  + len_b - 1, &chr_name_b, &chr_len_b, &chr_beg_b, &chr_end_b);
			
				if (strcmp(chr_name_f, chr_name_b) == 0 and chr_beg_b + len_b <= chr_beg_f + GENETHRESH) {
					mrl[mrl_size].is_concord = true;
					mrl[mrl_size].chr = chr_name_f;
					mrl[mrl_size].start_pos = chr_beg_f;
					mrl[mrl_size].end_pos = chr_end_b;
					mrl[mrl_size].matched_len = spos_back + len_b - spos_front + 1;
					mrl[mrl_size].dir = 1;
					if (++mrl_size >= MRLSIZELIM)
						return 1;
				}
			}
		}
		else {
			for (j = 0; j < flist_size_b; j++) {
				target = forwardlist_b[j] - len_f;
				if (target < backwardlist_f[0])	// no position less than forwardlist_b[j] available on backwardlist_f
					continue;
				else {
					i = binary_search(backwardlist_f, 0, blist_size_f, target);
				}
				spos_front = backwardlist_f[i-1];
				spos_back = forwardlist_b[j];
				//fprintf(stderr, "---Start pos front: %llu\n---Start pos back: %llu\n Dir front: %d\n Dir back: %d\n", spos_front, spos_back, dir_front, dir_back);
				
				bwt_get_intv_info(spos_front, spos_front + len_f - 1, &chr_name_f, &chr_len_f, &chr_beg_f, &chr_end_f);
				bwt_get_intv_info(spos_back,  spos_back  + len_b - 1, &chr_name_b, &chr_len_b, &chr_beg_b, &chr_end_b);
			
				if (strcmp(chr_name_f, chr_name_b) == 0 and chr_beg_b + len_b <= chr_beg_f + GENETHRESH) {
					mrl[mrl_size].is_concord = true;
					mrl[mrl_size].chr = chr_name_f;
					mrl[mrl_size].start_pos = chr_beg_f;
					mrl[mrl_size].end_pos = chr_end_b;
					mrl[mrl_size].matched_len = spos_back + len_b - spos_front + 1;
					mrl[mrl_size].dir = 1;
					if (++mrl_size >= MRLSIZELIM)
						return 1;
				}
			}
		}
	}

	if (mrl_size > 0)	// at least one concordant explanation found
		return 1;

	bool is_chimeric = is_chimeric_intersect(forwardlist_f, flist_size_f, backwardlist_f, blist_size_f, len_f, forwardlist_b, flist_size_b, backwardlist_b, blist_size_b, len_b);
	if (is_chimeric)	// does not keep chimeric match record
		return 2;
	mrl_size = 0;
	
	//if (trig == 0)
	//	sprintf(keep_log1, "Discordant\t%llu\t%llu\t%llu\t%llu\t", flist_size_f, blist_size_f, flist_size_b, blist_size_b);
	//else
	//	sprintf(keep_log2, "Discordant\t%llu\t%llu\t%llu\t%llu\t", flist_size_f, blist_size_f, flist_size_b, blist_size_b);
	return 3;
}

// return value:
// 0 : concordant (exon)
// 1 : concordant (junction)
// 2 : chimeric
// 3 : discordant
// 4 : potentially mappable
// 5 : un-mappable
int find_expanded_sliding_positions2(const char* rseq, const char* rcseq, const int& rseq_len, const int& window_size, const int& step, const int& junction_detect_size_lim, vector <MatchedRead>& mrl, int& mrl_size, bwtint_t& sp_b, bwtint_t& ep_b, int& exp_len_back) {
	sp_b = 0; 
	ep_b = 0;
	bwtint_t sp_f, ep_f;
	bwtint_t sapos, tmp_pos;
	exp_len_back = 0;
	int exp_len_front = 0;
	int dir;
	int i;
	int intersect_ret;

	char *chr_name;
	int32_t chr_len;
	uint32_t chr_beg;
	uint32_t chr_end;

	vector <ExtendMatch> extended_match(rseq_len);
	for (i = 0; i < rseq_len - window_size; i += step) {
		exp_len_front = get_expanded_locs(rcseq, rseq_len - i, &sp_f, &ep_f);
		if (exp_len_front < window_size) {
			extended_match[i].sp = sp_f;
			extended_match[i].ep = ep_f;
			extended_match[i].matched_len = exp_len_front;
			extended_match[i].end_ind = i + exp_len_front - 1;
			continue;
		}

		print_location_list(verboseMode, sp_f, ep_f, exp_len_front);

		if (i < junction_detect_size_lim and rseq_len - exp_len_front - i < junction_detect_size_lim) {	// concordant (exon)
			vafprintf(verboseMode, stderr, "[Concordant-e]\tfront matched: %d\n", exp_len_front);
			
			mrl_size = 0;
			for (sapos = sp_f; sapos <= ep_f; ++sapos) {
				mrl[mrl_size].is_concord = true;
				
				tmp_pos = get_pos(&sapos, exp_len_front, dir);
				bwt_get_intv_info(tmp_pos, tmp_pos + exp_len_front - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
				
				mrl[mrl_size].chr = chr_name;
				mrl[mrl_size].start_pos = chr_beg;
				mrl[mrl_size].end_pos = chr_end;
				mrl[mrl_size].dir = -1 * dir;
				mrl[mrl_size].matched_len = exp_len_front;
				if (++mrl_size >= MRLSIZELIM)
					break;
			}
			return 0;
		}
		break;
	}

	if (exp_len_front < window_size) {	// un-mappable (i > rseq_len - window_size)
		mrl_size = 0;
		return 5;
	}

	const char* remain_rseq = rseq + i + exp_len_front + 1;	// skipping one bp for noise/mismatch
	int remain_len = rseq_len - i - exp_len_front - 1;
	vafprintf(verboseMode, stderr, "Remaining len: %d\n", remain_len);
	
	bwtint_t longest_sp_f, longest_ep_f;
	int max_len_front = 0;
	if (remain_len < junction_detect_size_lim) {	// won't be able to find an event from the remaining of the read (potentially mappable)
		
		// intersect with longest non-overlapping match of previous part
		int l = -1;
		for (int k = 0; k < i; k += step) { 	// k < i since we do not consider the last match
			if (extended_match[k].end_ind < i and extended_match[k].matched_len > max_len_front) {
				max_len_front = extended_match[k].matched_len;
				longest_sp_f = extended_match[k].sp;
				longest_ep_f = extended_match[k].ep;
				l = k;
			}
		}
		
		if (max_len_front < junction_detect_size_lim) {
			mrl_size = 0;
			for (sapos = sp_f; sapos <= ep_f; ++sapos) {
				mrl[mrl_size].is_concord = false;
				
				tmp_pos = get_pos(&sapos, exp_len_front, dir);
				bwt_get_intv_info(tmp_pos, tmp_pos + exp_len_front - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
				
				mrl[mrl_size].chr = chr_name;
				mrl[mrl_size].start_pos = chr_beg;
				mrl[mrl_size].end_pos = chr_end;
				mrl[mrl_size].dir = -1 * dir;
				mrl[mrl_size].matched_len = exp_len_front;
				if (++mrl_size >= MRLSIZELIM)
					break;
			}
			return 4;
		}
		// intersect
		vafprintf(verboseMode, stderr, "Intersecting same strand:\n");
		vafprintf(verboseMode, stderr, "start index: %d\n", l);
		print_location_list(verboseMode, longest_sp_f, longest_ep_f, max_len_front);
		intersect_ret = intersect(longest_sp_f, longest_ep_f, max_len_front, sp_f, ep_f, exp_len_front, mrl, mrl_size, true);

		sp_b = sp_f;
		ep_b = ep_f;
		exp_len_back = exp_len_front;
		return intersect_ret;
	}

	int j = 0;
	bwtint_t longest_sp_b, longest_ep_b;
	int max_len_back = 0;
	int max_end_ind = 0;
	for (j = 0; j <= remain_len - max_len_back; j += step) {
		exp_len_back = get_expanded_locs(remain_rseq, remain_len - j, &sp_b, &ep_b);
		vafprintf(verboseMode, stderr, "%s\tlen front: %d,\tlen back: %d,\tj: %d\n", rseq, exp_len_front, exp_len_back, j);
		//print_location_list(verboseMode, sp_b, ep_b, exp_len_back);
	
		if (exp_len_back >= max_len_back) {
			max_len_back = exp_len_back;
			longest_sp_b = sp_b;
			longest_ep_b = ep_b;
			//max_end_ind = j + exp_len_back;
		}
		//if (exp_len_back >= window_size)
		//	break;
	}
	
	if (max_len_back <= junction_detect_size_lim) {	// not long enough for detecting junction
		mrl_size = 0;
		for (sapos = sp_f; sapos <= ep_f; ++sapos) {
			mrl[mrl_size].is_concord = false;
			tmp_pos = get_pos(&sapos, exp_len_front, dir);
			bwt_get_intv_info(tmp_pos, tmp_pos + exp_len_front - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
			
			mrl[mrl_size].chr = chr_name;
			mrl[mrl_size].start_pos = chr_beg;
			mrl[mrl_size].end_pos = chr_end;
			mrl[mrl_size].dir = -1 * dir;
			mrl[mrl_size].matched_len = exp_len_front;
			if (++mrl_size >= MRLSIZELIM)
				break;
		}
		return 4;
	}
	//bool consis = check_cosistent_match(&sp_b, &ep_b, exp_len_backward, &sp_f, &ep_f, exp_len_forward);
	//bool consis = is_concordant(sp_f, ep_f, exp_len_front, sp_b, ep_b, exp_len_back, noise_thresh);
	//bool consis = is_concordant_sorted2(sp_f, ep_f, exp_len_front, longest_sp_b, longest_ep_b, exp_len_back, 10, mr);
	
	vafprintf(verboseMode, stderr, "Intersecting different strand:\n");
	print_location_list(verboseMode, longest_sp_b, longest_ep_b, max_len_back);
	intersect_ret = intersect(sp_f, ep_f, exp_len_front, longest_sp_b, longest_ep_b, max_len_back, mrl, mrl_size, false);
	
	sp_b = longest_sp_b;
	ep_b = longest_ep_b;
	exp_len_back = max_len_back;
	return intersect_ret;
}

// return value:
// 0 : concordant (exon)
// 1 : concordant (junction)
// 2 : chimeric
// 3 : discordant
// 4 : potentially mappable
// 5 : un-mappable
int find_expanded_sliding_positions(const char* rseq, const char* rcseq, const int& rseq_len, const int& window_size, const int& step, const int& junction_detect_size_lim, vector <MatchedRead>& mrl, int& mrl_size) {
	bwtint_t sp_b, ep_b;
	bwtint_t sp_f, ep_f;
	bwtint_t sapos, tmp_pos;
	int exp_len_back = 0;
	int exp_len_front = 0;
	int dir;
	int i;
	int intersect_ret;

	char *chr_name;
	int32_t chr_len;
	uint32_t chr_beg;
	uint32_t chr_end;

	vector <ExtendMatch> extended_match(rseq_len);
	for (i = 0; i < rseq_len - window_size; i += step) {
		exp_len_front = get_expanded_locs(rcseq, rseq_len - i, &sp_f, &ep_f);
		if (exp_len_front < window_size) {
			extended_match[i].sp = sp_f;
			extended_match[i].ep = ep_f;
			extended_match[i].matched_len = exp_len_front;
			extended_match[i].end_ind = i + exp_len_front - 1;
			continue;
		}

		print_location_list(verboseMode, sp_f, ep_f, exp_len_front);

		if (i < junction_detect_size_lim and rseq_len - exp_len_front - i < junction_detect_size_lim) {	// concordant (exon)
			vafprintf(verboseMode, stderr, "[Concordant-e]\tfront matched: %d\n", exp_len_front);
			
			mrl_size = 0;
			for (sapos = sp_f; sapos <= ep_f; ++sapos) {
				mrl[mrl_size].is_concord = true;
				
				tmp_pos = get_pos(&sapos, exp_len_front, dir);
				bwt_get_intv_info(tmp_pos, tmp_pos + exp_len_front - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
				
				mrl[mrl_size].chr = chr_name;
				mrl[mrl_size].start_pos = chr_beg;
				mrl[mrl_size].end_pos = chr_end;
				mrl[mrl_size].dir = -1 * dir;
				mrl[mrl_size].matched_len = exp_len_front;
				if (++mrl_size >= MRLSIZELIM)
					break;
			}
			return 0;
		}
		break;
	}

	if (exp_len_front < window_size) {	// un-mappable (i > rseq_len - window_size)
		mrl_size = 0;
		return 5;
	}

	const char* remain_rseq = rseq + i + exp_len_front + 1;	// skipping one bp for noise/mismatch
	int remain_len = rseq_len - i - exp_len_front - 1;
	vafprintf(verboseMode, stderr, "Remaining len: %d\n", remain_len);
	
	bwtint_t longest_sp_f, longest_ep_f;
	int max_len_front = 0;
	if (remain_len < junction_detect_size_lim) {	// won't be able to find an event from the remaining of the read (potentially mappable)
		
		// intersect with longest non-overlapping match of previous part
		int l = -1;
		for (int k = 0; k < i; k += step) { 	// k < i since we do not consider the last match
			if (extended_match[k].end_ind < i and extended_match[k].matched_len > max_len_front) {
				max_len_front = extended_match[k].matched_len;
				longest_sp_f = extended_match[k].sp;
				longest_ep_f = extended_match[k].ep;
				l = k;
			}
		}
		
		if (max_len_front < junction_detect_size_lim) {
			mrl_size = 0;
			for (sapos = sp_f; sapos <= ep_f; ++sapos) {
				mrl[mrl_size].is_concord = false;
				
				tmp_pos = get_pos(&sapos, exp_len_front, dir);
				bwt_get_intv_info(tmp_pos, tmp_pos + exp_len_front - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
				
				mrl[mrl_size].chr = chr_name;
				mrl[mrl_size].start_pos = chr_beg;
				mrl[mrl_size].end_pos = chr_end;
				mrl[mrl_size].dir = -1 * dir;
				mrl[mrl_size].matched_len = exp_len_front;
				if (++mrl_size >= MRLSIZELIM)
					break;
			}
			return 4;
		}
		// intersect
		vafprintf(verboseMode, stderr, "Intersecting same strand:\n");
		vafprintf(verboseMode, stderr, "start index: %d\n", l);
		print_location_list(verboseMode, longest_sp_f, longest_ep_f, max_len_front);
		intersect_ret = intersect(longest_sp_f, longest_ep_f, max_len_front, sp_f, ep_f, exp_len_front, mrl, mrl_size, true);
		return intersect_ret;
	}

	int j = 0;
	bwtint_t longest_sp_b, longest_ep_b;
	int max_len_back = 0;
	int max_end_ind = 0;
	for (j = 0; j <= remain_len - max_len_back; j += step) {
		exp_len_back = get_expanded_locs(remain_rseq, remain_len - j, &sp_b, &ep_b);
		vafprintf(verboseMode, stderr, "%s\tlen front: %d,\tlen back: %d,\tj: %d\n", rseq, exp_len_front, exp_len_back, j);
		//print_location_list(verboseMode, sp_b, ep_b, exp_len_back);
	
		if (exp_len_back >= max_len_back) {
			max_len_back = exp_len_back;
			longest_sp_b = sp_b;
			longest_ep_b = ep_b;
			//max_end_ind = j + exp_len_back;
		}
		//if (exp_len_back >= window_size)
		//	break;
	}
	
	if (max_len_back <= junction_detect_size_lim) {	// not long enough for detecting junction
		mrl_size = 0;
		for (sapos = sp_f; sapos <= ep_f; ++sapos) {
			mrl[mrl_size].is_concord = false;
			tmp_pos = get_pos(&sapos, exp_len_front, dir);
			bwt_get_intv_info(tmp_pos, tmp_pos + exp_len_front - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
			
			mrl[mrl_size].chr = chr_name;
			mrl[mrl_size].start_pos = chr_beg;
			mrl[mrl_size].end_pos = chr_end;
			mrl[mrl_size].dir = -1 * dir;
			mrl[mrl_size].matched_len = exp_len_front;
			if (++mrl_size >= MRLSIZELIM)
				break;
		}
		return 4;
	}
	//bool consis = check_cosistent_match(&sp_b, &ep_b, exp_len_backward, &sp_f, &ep_f, exp_len_forward);
	//bool consis = is_concordant(sp_f, ep_f, exp_len_front, sp_b, ep_b, exp_len_back, noise_thresh);
	//bool consis = is_concordant_sorted2(sp_f, ep_f, exp_len_front, longest_sp_b, longest_ep_b, exp_len_back, 10, mr);
	
	vafprintf(verboseMode, stderr, "Intersecting different strand:\n");
	print_location_list(verboseMode, longest_sp_b, longest_ep_b, max_len_back);
	intersect_ret = intersect(sp_f, ep_f, exp_len_front, longest_sp_b, longest_ep_b, max_len_back, mrl, mrl_size, false);

	return intersect_ret;
}

// return value:
// 0 : concordant (exon)
// 1 : concordant (junction)
// 2 : chimeric
// 3 : discordant
// 4 : potentially mappable
// 5 : un-mappable
//int find_expanded_sliding_positions(const char* rseq, const char* rcseq, const int& rseq_len, const int& window_size, const int& step, const int& junction_detect_size_lim, vector <MatchedRead>& mrl, int& mrl_size) {
//	bwtint_t sp_b, ep_b;
//	bwtint_t sp_f, ep_f;
//	bwtint_t sapos, tmp_pos;
//	int exp_len_back = 0;
//	int exp_len_front = 0;
//	int dir;
//	int i;
//	int intersect_ret;
//
//	char *chr_name;
//	int32_t chr_len;
//	uint32_t chr_beg;
//	uint32_t chr_end;
//
//	vector <ExtendMatch> extended_match(rseq_len);
//	for (i = 0; i < rseq_len - window_size; i += step) {
//		exp_len_front = get_expanded_locs(rcseq, rseq_len - i, &sp_f, &ep_f);
//		if (exp_len_front < window_size) {
//			extended_match[i].sp = sp_f;
//			extended_match[i].ep = ep_f;
//			extended_match[i].matched_len = exp_len_front;
//			extended_match[i].end_ind = i + exp_len_front - 1;
//			continue;
//		}
//
//		print_location_list(verboseMode, sp_f, ep_f, exp_len_front);
//
//		if (i < junction_detect_size_lim and rseq_len - exp_len_front - i < junction_detect_size_lim) {	// concordant (exon)
//			vafprintf(verboseMode, stderr, "[Concordant-e]\tfront matched: %d\n", exp_len_front);
//			
//			mrl_size = 0;
//			for (sapos = sp_f; sapos <= ep_f; ++sapos) {
//				mrl[mrl_size].is_concord = true;
//				
//				tmp_pos = get_pos(&sapos, exp_len_front, dir);
//				bwt_get_intv_info(tmp_pos, tmp_pos + exp_len_front - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
//				
//				mrl[mrl_size].chr = chr_name;
//				mrl[mrl_size].start_pos = chr_beg;
//				mrl[mrl_size].end_pos = chr_end;
//				mrl[mrl_size].dir = -1 * dir;
//				mrl[mrl_size].matched_len = exp_len_front;
//				if (++mrl_size >= MRLSIZELIM)
//					break;
//			}
//			return 0;
//		}
//		break;
//	}
//
//	if (exp_len_front < window_size) {	// un-mappable (i > rseq_len - window_size)
//		mrl_size = 0;
//		return 5;
//	}
//
//	const char* remain_rseq = rseq + i + exp_len_front + 1;	// skipping one bp for noise/mismatch
//	int remain_len = rseq_len - i - exp_len_front - 1;
//	vafprintf(verboseMode, stderr, "Remaining len: %d\n", remain_len);
//	
//	bwtint_t longest_sp_f, longest_ep_f;
//	int max_len_front = 0;
//	if (remain_len < junction_detect_size_lim) {	// won't be able to find an event from the remaining of the read (potentially mappable)
//		
//		// intersect with longest non-overlapping match of previous part
//		//int l = -1;
//		//for (int k = 0; k < i; k += step) { 	// k < i since we do not consider the last match
//		//	if (extended_match[k].end_ind < i and extended_match[k].matched_len > max_len_front) {
//		//		max_len_front = extended_match[k].matched_len;
//		//		longest_sp_f = extended_match[k].sp;
//		//		longest_ep_f = extended_match[k].ep;
//		//		l = k;
//		//	}
//		//}
//		
//		//if (max_len_front < junction_detect_size_lim) {
//			mrl_size = 0;
//			for (sapos = sp_f; sapos <= ep_f; ++sapos) {
//				mrl[mrl_size].is_concord = false;
//				
//				tmp_pos = get_pos(&sapos, exp_len_front, dir);
//				bwt_get_intv_info(tmp_pos, tmp_pos + exp_len_front - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
//				
//				mrl[mrl_size].chr = chr_name;
//				mrl[mrl_size].start_pos = chr_beg;
//				mrl[mrl_size].end_pos = chr_end;
//				mrl[mrl_size].dir = -1 * dir;
//				mrl[mrl_size].matched_len = exp_len_front;
//				if (++mrl_size >= MRLSIZELIM)
//					break;
//			}
//			return 4;
//		//}
//		//// intersect
//		//vafprintf(verboseMode, stderr, "Intersecting same strand:\n");
//		//vafprintf(verboseMode, stderr, "start index: %d\n", l);
//		//print_location_list(verboseMode, longest_sp_f, longest_ep_f, max_len_front);
//		//intersect_ret = intersect(longest_sp_f, longest_ep_f, max_len_front, sp_f, ep_f, exp_len_front, mrl, mrl_size, true);
//		//return intersect_ret;
//	}
//
//	int j = 0;
//	bwtint_t longest_sp_b, longest_ep_b;
//	int max_len_back = 0;
//	int max_end_ind = 0;
//	//for (j = 0; j <= remain_len - max_len_back; j += step) {
//	for (j = 0; j <= remain_len - junction_detect_size_lim; j += step) {
//		exp_len_back = get_expanded_locs(remain_rseq, remain_len - j, &sp_b, &ep_b);
//		vafprintf(verboseMode, stderr, "%s\tlen front: %d,\tlen back: %d,\tj: %d\n", rseq, exp_len_front, exp_len_back, j);
//	
//		// intersect
//		if (exp_len_back > junction_detect_size_lim and j + exp_len_back > max_end_ind) {
//			vafprintf(verboseMode, stderr, "Intersecting different strand:\n");
//			print_location_list(verboseMode, sp_b, ep_b, exp_len_back);
//			intersect_ret = intersect(sp_f, ep_f, exp_len_front, sp_b, ep_b, exp_len_back, mrl, mrl_size, false);
//
//			vafprintf(verboseMode, stderr, "Intersection result: %d\n", intersect_ret);
//			if (intersect_ret == 1 or intersect_ret == 2)
//				return intersect_ret;
//		}
//
//		if (exp_len_back >= max_len_back) {
//			max_len_back = exp_len_back;
//			longest_sp_b = sp_b;
//			longest_ep_b = ep_b;
//			max_end_ind = j + exp_len_back;
//		}
//		//if (exp_len_back >= window_size)
//		//	break;
//	}
//	
//	if (max_len_back <= junction_detect_size_lim) {	// not long enough for detecting junction
//		mrl_size = 0;
//		for (sapos = sp_f; sapos <= ep_f; ++sapos) {
//			mrl[mrl_size].is_concord = false;
//			tmp_pos = get_pos(&sapos, exp_len_front, dir);
//			bwt_get_intv_info(tmp_pos, tmp_pos + exp_len_front - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
//			
//			mrl[mrl_size].chr = chr_name;
//			mrl[mrl_size].start_pos = chr_beg;
//			mrl[mrl_size].end_pos = chr_end;
//			mrl[mrl_size].dir = -1 * dir;
//			mrl[mrl_size].matched_len = exp_len_front;
//			if (++mrl_size >= MRLSIZELIM)
//				break;
//		}
//		return 4;
//	}
//	//bool consis = check_cosistent_match(&sp_b, &ep_b, exp_len_backward, &sp_f, &ep_f, exp_len_forward);
//	//bool consis = is_concordant(sp_f, ep_f, exp_len_front, sp_b, ep_b, exp_len_back, noise_thresh);
//	//bool consis = is_concordant_sorted2(sp_f, ep_f, exp_len_front, longest_sp_b, longest_ep_b, exp_len_back, 10, mr);
//	
//	//vafprintf(verboseMode, stderr, "Intersecting different strand:\n");
//	//print_location_list(verboseMode, longest_sp_b, longest_ep_b, max_len_back);
//	//intersect_ret = intersect(sp_f, ep_f, exp_len_front, longest_sp_b, longest_ep_b, max_len_back, mrl, mrl_size, false);
//
//	//return intersect_ret;
//	
//	return 3;
//}

// return value:
// 0 : concordant (exon)
// 1 : concordant (junction)
// 2 : chimeric (back splice junction)
// 3 : discordant
// 4 : potentially mappable
// 5 : un-mappable
// 6 : chimeric (fusion)
int check_concordant_mates_expand(const Record* m1, const Record* m2, int kmer_size) {
	int junction_detect_size_lim = 9;
	MatchedRead mr1, mr2;
	//MatchedReadList mrl1, mrl2;
	vector <MatchedRead> mrl1(MRLSIZELIM);
	vector <MatchedRead> mrl2(MRLSIZELIM);
	int mrl1_size, mrl2_size;
	vector <MatchedRead> mrl1_rc(MRLSIZELIM);
	vector <MatchedRead> mrl2_rc(MRLSIZELIM);
	int mrl1_rc_size, mrl2_rc_size;

	int mate1_state, mate2_state;
	int mate1_rc_state = -1, mate2_rc_state = -1;

	vafprintf(verboseMode, stderr, "Read name: %s1st mate:\n", m1->rname);
	
	////
	string rname = m1->rname;
	int size = rname.length();
	rname[size-1] = '\0';
	fprintf(outputJuncFile, "%s\\R1\t", rname.c_str());
	////
	
	trig = 0;
	//mate1_state = find_expanded_sliding_positions(m1->seq, m1->rcseq, m1->seq_len, kmer_size, 3, junction_detect_size_lim, mrl1, mrl1_size);
	bwtint_t sp_b, ep_b;
	int len_b;
	bwtint_t sp_b_rc, ep_b_rc;
	int len_b_rc;
	mate1_state = find_expanded_sliding_positions2(m1->seq, m1->rcseq, m1->seq_len, kmer_size, 3, junction_detect_size_lim, mrl1, mrl1_size, sp_b, ep_b, len_b);
	if (mate1_state == 2 or mate1_state == 3 or mate1_state == 4) {
	//if (mate1_state == 3 or mate1_state == 4) {
		mate1_rc_state = find_expanded_sliding_positions2(m1->rcseq, m1->seq, m1->seq_len, kmer_size, 3, junction_detect_size_lim, mrl1_rc, mrl1_rc_size, sp_b_rc, ep_b_rc, len_b_rc);
		
		// deciding mate1 state based on seq map and its rc map
		if (mate1_rc_state == 0 or mate1_rc_state == 1) {
			mate1_state = mate1_rc_state;
			mrl1_size = mrl1_rc_size;
			mrl1 = mrl1_rc;
			for (int l = 0; l < mrl1_size; l++)
				mrl1[l].dir *= -1;
		}
		else if (mate1_state == 2 or mate1_rc_state == 2) {
			mate1_state = 2;
		}
		else if (mate1_state == 3 and mate1_rc_state == 3) {
			// intersect back to back (from normal and RC of read)
			vafprintf(verboseMode, stderr, "--B2B intersection:\n");
			int intersect_ret = intersect(sp_b_rc, ep_b_rc, len_b_rc, sp_b, ep_b, len_b, mrl1, mrl1_size, false);
			mate1_state = intersect_ret;
		}
		else if (mate1_state == 3 or mate1_rc_state == 3) {
			mate1_state = 3;
		}
		else if (mate1_state == 4 or mate1_rc_state == 4) {
			mate1_state = 4;
		}
	}

	////
	if (mate1_state == 2)
		fprintf(outputJuncFile, "\n");
	else if (mate1_state < 2) {
		string mate_strand = (mrl1[0].dir == 1) ? "+" : "-";
		fprintf(outputJuncFile, "%s\t%d\t%d\t%s\n", mrl1[0].chr, mrl1[0].start_pos, mrl1[0].start_pos + mrl1[0].matched_len, mate_strand.c_str());
	}
	else 
		fprintf(outputJuncFile, "-\t-\t-\t-\n", m1->rname);
	////

	vafprintf(verboseMode, stderr, "2nd mate:\n");

	////
	fprintf(outputJuncFile, "%s\\R2\t", rname.c_str());
	////
	
	trig = 1;
	mate2_state = find_expanded_sliding_positions2(m2->seq, m2->rcseq, m2->seq_len, kmer_size, 3, junction_detect_size_lim, mrl2, mrl2_size, sp_b, ep_b, len_b);
	if (mate2_state == 2 or mate2_state == 3 or mate2_state == 4) {
	//if (mate2_state == 3 or mate2_state == 4) {
		mate2_rc_state = find_expanded_sliding_positions2(m2->rcseq, m2->seq, m2->seq_len, kmer_size, 3, junction_detect_size_lim, mrl2_rc, mrl2_rc_size, sp_b_rc, ep_b_rc, len_b_rc);
		
		// deciding mate2 state based on seq map and its rc map
		if (mate2_rc_state == 0 or mate2_rc_state == 1) {
			mate2_state = mate2_rc_state;
			mrl2_size = mrl2_rc_size;
			mrl2 = mrl2_rc;
			for (int l = 0; l < mrl2_size; l++)
				mrl2[l].dir *= -1;
		}
		else if (mate2_state == 2 or mate2_rc_state == 2) {
			mate2_state = 2;
		}
		else if (mate2_state == 3 and mate2_rc_state == 3) {
			// intersect back to back (from normal and RC of read)
			vafprintf(verboseMode, stderr, "--B2B intersection:\n");
			int intersect_ret = intersect(sp_b_rc, ep_b_rc, len_b_rc, sp_b, ep_b, len_b, mrl2, mrl2_size, false);
			mate2_state = intersect_ret;
		}
		else if (mate2_state == 3 or mate2_rc_state == 3) {
			mate2_state = 3;
		}
		else if (mate2_state == 4 or mate2_rc_state == 4) {
			mate2_state = 4;
		}
	}

	////
	if (mate2_state == 2)
		fprintf(outputJuncFile, "\n");
	else if (mate2_state < 2) {
		string mate_strand = (mrl2[0].dir == 1) ? "+" : "-";
		fprintf(outputJuncFile, "%s\t%d\t%d\t%s\n", mrl2[0].chr, mrl2[0].start_pos, mrl2[0].start_pos + mrl2[0].matched_len, mate_strand.c_str());
	}
	else 
		fprintf(outputJuncFile, "-\t-\t-\t-\n");
	////

	vafprintf(verboseMode, stderr, "%d\n%d\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n", mate1_state, mate2_state);

	if (mate1_state == 2 or mate2_state == 2) {
		//if (mate1_state == 2)
		//	return mate2_state;
		//if (mate2_state == 2)
		//	return mate1_state;

		return 2;
	}

	if (mate1_state == 3 or mate2_state == 3) {
		//if (mate1_state == 3)
		//	fprintf(stderr, "%s\t", keep_log1);
		//if (mate2_state == 3)
		//	fprintf(stderr, "%s\t", keep_log2);
		//fprintf(stderr, "%s", m1->rname);
		return 3;
	}

	if (mate1_state == 5)
		return mate2_state;
	if (mate2_state == 5)
		return mate1_state;

	if (mate1_state == 4 or mate2_state == 4)
		return 4;

	// check for any concordancy evidence/explanation
	for (int i = 0; i < mrl1_size; i++)
		for (int j = 0; j < mrl2_size; j++) {
			mr1 = mrl1[i];
			mr2 = mrl2[j];
			if (strcmp(mr1.chr, mr2.chr) != 0)
				continue;
			if (mr1.dir == 1 and mr2.dir == -1 and (mr1.start_pos <= mr2.start_pos))
				return 0;
			if (mr1.dir == -1 and mr2.dir == 1 and (mr2.start_pos <= mr1.start_pos))
				return 0;
		}

	bool same_chr_exists = false;
	// non-overlapping chimeric mates in the gene range
	for (int i = 0; i < mrl1_size; i++)
		for (int j = 0; j < mrl2_size; j++) {
			mr1 = mrl1[i];
			mr2 = mrl2[j];
			if (strcmp(mr1.chr, mr2.chr) != 0)
				continue;
			else
				same_chr_exists = true;

			// non-overlapping back scplice junction
			if (mr1.dir == 1 and mr2.dir == -1 and (mr1.start_pos > mr2.start_pos + mr2.matched_len) and (mr1.start_pos <= mr2.start_pos + GENETHRESH)) {
				//fprintf(stderr, "%s\t%d\t%d\t+\t%s\t%d\t%d\t-\tWrong Mate Orientation\t%s", mr1.chr, mr1.start_pos, mr1.end_pos, mr2.chr, mr2.start_pos, mr2.end_pos, m1->rname);
				return 2;
			}
			if (mr1.dir == -1 and mr2.dir == 1 and (mr2.start_pos > mr1.start_pos + mr1.matched_len) and (mr2.start_pos <= mr1.start_pos + GENETHRESH)) {
				//fprintf(stderr, "%s\t%d\t%d\t+\t%s\t%d\t%d\t-\tWrong Mate Orientation\t%s", mr2.chr, mr2.start_pos, mr2.end_pos, mr1.chr, mr1.start_pos, mr1.end_pos, m1->rname);
				return 2;
			}
		}

	return (same_chr_exists) ? 0 : 6;	// fusion if no occurance on same chr exists
}

void chop_read_match(const char* rseq, int rseq_len, int kmer_size, vector<fragment_t>& forward_fragments, int& forward_fragment_count, vector<fragment_t>& backward_fragments, int& backward_fragment_count) {
	bwtint_t sp, ep, j, rpos;
	int i, occ;
	int sum = 0;
	int dir;
	uint32_t match_len = kmer_size;
	int32_t end_pos;

	forward_fragment_count = 0;
	backward_fragment_count = 0;
	for (i = 0; i < rseq_len; i += kmer_size) {
		if (rseq_len - i < kmer_size)
			match_len = rseq_len - i;
		occ = get_exact_locs(rseq + i, match_len, &sp, &ep);
		if (occ <= 0)
			continue;

		sum += occ;
		if (sum >= FRAGLIM) {
			forward_fragment_count = 0;
			backward_fragment_count = 0;
			return;
		}

		//fprintf(stderr, "Occ: %d\tmatch len: %d\n", occ, match_len);

		for (j = sp; j <= ep; j++) {
			rpos = get_pos(&j, match_len, dir);
			if (dir == 1) {
				forward_fragments[forward_fragment_count].qpos = i;
				forward_fragments[forward_fragment_count].rpos = rpos;
				forward_fragments[forward_fragment_count].len = match_len;
				forward_fragment_count++;
			}
			else {
				end_pos = i + match_len - 1;
				backward_fragments[backward_fragment_count].qpos = -1 * end_pos;
				backward_fragments[backward_fragment_count].rpos = rpos;
				backward_fragments[backward_fragment_count].len = match_len;
				backward_fragment_count++;
			}
		}
	}
}


void print_location_list(int verbosity, const bwtint_t& sp, const bwtint_t& ep, const int& len) {
	if (verbosity < 1)	return;
	bwtint_t tmp;
	int dir;
	fprintf(stderr, "===\nLength of match: %d\n", len);
	for (bwtint_t i = sp; i <= ep; i++) {
		tmp = get_pos(&i, len, dir);
		fprintf(stderr, "strand: %d\t", dir);
		char *chr_name;
		int32_t chr_len;
		uint32_t chr_beg;
		uint32_t chr_end;
		bwt_get_intv_info(tmp, tmp + len - 1, &chr_name, &chr_len, &chr_beg, &chr_end);
		fprintf(stderr, "chr%s: ", chr_name);
		fprintf(stderr, "%"PRIu64"-", chr_beg);
		fprintf(stderr, "%"PRIu64"\t", chr_end);
	}
	fprintf(stderr, "\n===\n");
}

// verbose-aware fprintf
void vafprintf(int verbosity, FILE *stream, const char *format, ...) {
	if (verbosity < 1)	return;

	va_list args;
	va_start (args, format);
	vfprintf (stream, format, args);
	va_end (args);
}


unsigned long long find_occ_sum(const char* rseq, int rseq_len, const int& kmer_size) {
	bwtint_t sp_f, ep_f;
	bwtint_t sp_b, ep_b;
	int occ = 0, i, j;
	unsigned long long sum = 0;

	for (i = 0; i <= rseq_len - kmer_size; ++i) {
		occ = get_exact_locs(rseq + i, kmer_size, &sp_f, &ep_f);
		//fprintf(stderr, "Number of matches: %d\n", occ);

		if (occ > 0) {
			sum += occ;
		}
	}

	fprintf(stderr, "%llu\n", sum);

	return sum;
}
