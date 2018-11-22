#include <cstdio> 
#include <cstring>
#include "fastq_parser.h"

#define MAXLINESIZE 400

FASTQParser::FASTQParser (bool read_state) : read_state(read_state) { 
	input = NULL; 
}

FASTQParser::FASTQParser (char* filename, bool read_state) : read_state(read_state) {
	init(filename);
}

FASTQParser::~FASTQParser (void) {
	if (input != NULL) {
		close_file(input);
		
		free(current_record->rname);
		free(current_record->seq);
		free(current_record->rcseq);
		free(current_record->comment);
		free(current_record->qual);

		delete current_record;
	}
}

void FASTQParser::init (char* filename) {
	input = open_file(filename, "r");

	max_line_size = MAXLINESIZE;
	current_record = new Record;
	set_comp();

	current_record->rname = (char*) malloc(max_line_size);
	current_record->seq = (char*) malloc(max_line_size);
	current_record->rcseq = (char*) malloc(max_line_size);
	current_record->comment = (char*) malloc(max_line_size);
	current_record->qual = (char*) malloc(max_line_size);
}

Record* FASTQParser::get_next (void) {
	if (has_next() and read_next())
		return current_record;
	size += 1;
	return NULL;
}

bool FASTQParser::has_next (void) {
	char c = fgetc(input);
	if (c == EOF)
		return false;
	
	assert(c == '@');	// ensure FASTQ format 
	return true;
}

bool FASTQParser::read_next (void) {
	current_record->rname[0] = '@';	// already read it in has_next()
	char * skip_at = current_record->rname + 1;

	int rname_len = 1;
	rname_len += getline(&skip_at, &max_line_size, input);
	extract_map_info(skip_at);

	getline(&current_record->seq, &max_line_size, input);
	getline(&current_record->comment, &max_line_size, input);
	getline(&current_record->qual, &max_line_size, input);
	
	assert(current_record->comment[0] == '+');

	current_record->seq_len = strlen(current_record->seq) - 1;	// skipping newline at the end
	current_record->rname[rname_len - 1] = '\0';
	//current_record->seq[current_record->seq_len] = '\0';

	set_reverse_comp();

	return true;
}

void FASTQParser::set_comp (void) {
	comp['A'] = 'T';
	comp['C'] = 'G';
	comp['G'] = 'C';
	comp['T'] = 'A';
	comp['N'] = 'N';

	comp['a'] = 'T';
	comp['c'] = 'G';
	comp['g'] = 'C';
	comp['t'] = 'A';
	comp['n'] = 'N';
}

void FASTQParser::set_reverse_comp (void) {
	if (current_record == NULL) {
		fprintf(stderr, "No read loaded to be reverse completed\n");
		return;
	}

	int len = current_record->seq_len;
	char nt;
	for (int i = len-1; i >= 0; --i) {
		current_record->rcseq[len-i-1] = comp[current_record->seq[i]];
	}
	current_record->rcseq[len] = '\0';
}

void FASTQParser::extract_map_info(char* str) {
	// Returns first token  
	char *token = strtok(str, " "); 
    
	// Keep printing tokens while one of the 
	// delimiters present in str[]. 
	int i = 0;
	while (token != NULL) 
	{ 
		strcpy(tokens[i], token);
		token = strtok(NULL, " "); 
		++i;
	}

	// i == 1 iff there is no comment in the line
	int rname_len = (i == 1) ? strlen(tokens[0]) : strlen(tokens[0]) + 1;
	current_record->rname[rname_len] = '\0';

	fill_map_info(i);
}

void FASTQParser::fill_map_info(int cnt) {
	assert(cnt == 1 or cnt == 12);
	
	if (cnt == 1) {
		current_record->mr.type = NOPROC_NOMATCH;
		current_record->mr.tlen = INF;
		current_record->mr.junc_num = 0;
		current_record->mr.gm_compatible = false;
	}
	else {
		char* stop_string;
		int base = 10;
		current_record->mr.type 	= atoi(tokens[1]);
		current_record->mr.chr 		= tokens[2];
		current_record->mr.spos_r1 	= strtoul(tokens[3], &stop_string, base);
		current_record->mr.epos_r1 	= strtoul(tokens[4], &stop_string, base);
		current_record->mr.mlen_r1 	= atoi(tokens[5]);
		current_record->mr.spos_r2 	= strtoul(tokens[6], &stop_string, base);
		current_record->mr.epos_r2 	= strtoul(tokens[7], &stop_string, base);
		current_record->mr.mlen_r2 	= atoi(tokens[8]);
		current_record->mr.tlen 	= atoi(tokens[9]);
		current_record->mr.junc_num = strtoul(tokens[10], &stop_string, base);
		current_record->mr.gm_compatible = (tokens[11][0] == '1');
	}
}