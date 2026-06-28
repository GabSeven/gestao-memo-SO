#include <algorithm>
#include <cctype>
#include <cstddef>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <vector>
#include <unordered_map>
#include <array>
#include <optional>
#include <algorithm>
#include <memory>
#include <map>

using namespace std;

#define TAM_MAX_LINHA 50
#define TAM_MEMORIA 4096

#define OP_ALLOC 	1
#define OP_ACCESS 	2
#define OP_FREE 	3

#define ERROR_VALUE -1

unordered_map<string, int> idProcesso;
unordered_map<string, unsigned int> idOperacao;
vector<string> nomeProcesso;

typedef struct Bloco {
	bool free = true;
	int id = 0;
} Bloco;

typedef struct Requisicao {
	unsigned int op: 2;
	int pid;
	optional<int> param;
} Requisicao;

typedef struct Particao {
	int base;
	int limite;
}Particao;

int finalizar_com_erro(const string mensagem) {
	cerr << mensagem << endl;
	exit(EXIT_FAILURE);
}

typedef enum {
	FIRST_FIT,
	BEST_FIT,
	WORST_FIT,
	BUDDY
} Estrategia;

class MMU {
public:
	static int limiteFisico(Particao p){
		return p.base + p.limite - 1;
	};
	
	static int endLogico_to_Fisico(Particao p, int endLog){
		if (p.limite <= endLog) {
			return ERROR_VALUE;
		};

		return p.base + endLog;
	};
};

class EstrategiaAlocacao {
protected:
	array<Bloco, TAM_MEMORIA> &memoria;
	unordered_map<int, Particao> &tabelaParticao;
	ofstream &log;
		
	virtual tuple<int, int> aloca(int pid, int ua) = 0;

	virtual tuple<int, int> libera(int pid);
	tuple<int, int> acessa(int pid, int endLog);
	
public:
	EstrategiaAlocacao(array<Bloco, TAM_MEMORIA> &mem, unordered_map<int, Particao> &tab, ofstream &logFile)
	: memoria(mem), tabelaParticao(tab), log(logFile) {};
	virtual ~EstrategiaAlocacao() = default;


	int executaRequisicao(Requisicao req, ofstream & log);
};


class FirstFit : public EstrategiaAlocacao {
public:
	FirstFit(array<Bloco, TAM_MEMORIA> &mem, unordered_map<int, Particao> &tab, ofstream &log)
	: EstrategiaAlocacao(mem, tab, log) {};

protected:
	tuple<int, int> aloca(int pid, int ua);
};


class BestFit : public EstrategiaAlocacao {
public:
	BestFit(array<Bloco, TAM_MEMORIA> &mem, unordered_map<int, Particao> &tab, ofstream &log)
	: EstrategiaAlocacao(mem, tab, log) {};

protected:
	tuple<int, int> aloca(int pid, int ua);
};


class WorstFit : public EstrategiaAlocacao {
public:
	WorstFit(array<Bloco, TAM_MEMORIA> &mem, unordered_map<int, Particao> &tab, ofstream &log)
	: EstrategiaAlocacao(mem, tab, log) {};

protected:
	tuple<int, int> aloca(int pid, int ua);
};


class Buddy : public EstrategiaAlocacao {
private:
	// tamanho -> posicao
	multimap<int, int> buddyMemo;
	map<int, set<int>> bM;
public:
	Buddy(array<Bloco, TAM_MEMORIA> &mem, unordered_map<int, Particao> &tab, ofstream &log)
	: EstrategiaAlocacao(mem, tab, log) {
		buddyMemo.insert({TAM_MEMORIA, 0});
	}

protected:
	tuple<int, int> aloca(int pid, int ua);
	tuple<int, int> libera(int pid);
};


class Simulador {
private:
	ofstream &log;
	array<Bloco, TAM_MEMORIA> memoria;
	unordered_map<int, Particao> tabelaParticao;
	unique_ptr<EstrategiaAlocacao> estrategiaAloc;

public:	
	Simulador(ofstream &logFile, Estrategia estrat) :log(logFile) {
		switch (estrat) {
			case FIRST_FIT:
				estrategiaAloc = make_unique<FirstFit>(memoria, tabelaParticao, log);
				break;
			case BEST_FIT:
				estrategiaAloc = make_unique<BestFit>(memoria, tabelaParticao, log);
				break;
			case WORST_FIT:
				estrategiaAloc = make_unique<WorstFit>(memoria, tabelaParticao, log);
				break;
			case BUDDY:
				estrategiaAloc = make_unique<Buddy>(memoria, tabelaParticao, log);
				break;
		}

	};

	void realizarRequisicao(Requisicao req){
		estrategiaAloc->executaRequisicao(req, log);
	};

	void simular(vector<Requisicao> requisicoes){
		for (const auto & req: requisicoes) realizarRequisicao(req);
	};
};


int EstrategiaAlocacao::executaRequisicao(Requisicao req, ofstream & log){
	if (req.op == OP_ALLOC){
		auto [inicio, fim] = aloca(req.pid, req.param.value());

		log << "alocacao " << nomeProcesso[req.pid];
		if (inicio == ERROR_VALUE){
			log << " erro!" << endl;
			return finalizar_com_erro("Nao foi possivel realizar a alocacao");
		}

		log << " "  << inicio << " " << fim << endl;
		
	} else if (req.op == OP_ACCESS) {
		auto [endLogico, endFisico] = acessa(req.pid, req.param.value());
		
		log << "acesso " << nomeProcesso[req.pid]
		<< " "<< endLogico << " ";
		
		if (endFisico == ERROR_VALUE) {
			log << "violacao" << endl;
		} else {
			log << endFisico << endl;
		}

	} else if (req.op == OP_FREE){
		auto [inicio, fim] = libera(req.pid);

		log << "liberacao " << nomeProcesso[req.pid] << " "
		<< inicio << " " << fim << endl;
	}
	return 0;
};

tuple<int, int> EstrategiaAlocacao::libera(int pid){
	Particao p = tabelaParticao[pid];
	int inicio = p.base;
	int fim = MMU::limiteFisico(p);
	
	for (int i = inicio; i <= fim; i++) {
		memoria[i] = {.free = true, .id = 0};
	}
	tabelaParticao.erase(pid);
	
	return {inicio, fim};
};

tuple<int, int> EstrategiaAlocacao::acessa(int pid, int endLog){
	Particao p = tabelaParticao[pid];
	int endFisico = MMU::endLogico_to_Fisico(p, endLog);
	return {endLog, endFisico};
};

tuple<int, int> FirstFit::aloca(int pid, int ua) {
	int inicio = 0;
	int tam = 0;
	for (int i = 0; i < TAM_MEMORIA && tam < ua; i++) {
		if (memoria[i].free){
			tam++;
		} else {
			inicio = i+1;
			tam = 0;
		}
	}
	
	if (tam != ua) {
		return {ERROR_VALUE, 0};
	}
	
	tabelaParticao[pid] = {inicio, ua};
	int fim = MMU::limiteFisico(tabelaParticao[pid]);
	for (int i = inicio; i <= fim; i++){
		memoria[i] = {.free= false, .id = pid};
	}

	return {inicio, fim};
};

tuple<int, int> BestFit::aloca(int pid, int ua) {
	int inicioAtual = 0, inicioMin = 0;
	int tamAtual = 0, tamMin = 0;
	for (int i = 0; i < TAM_MEMORIA; i++) {
		if (memoria[i].free){
			tamAtual++;
		} else{
			if (tamMin == 0 || (tamAtual >= ua && tamAtual < tamMin)){
				tamMin = tamAtual;
				inicioMin = inicioAtual;
			}
			tamAtual = 0;
			inicioAtual = i + 1;
		} 
	}

	if (tamMin == 0 || (tamAtual >= ua && tamAtual < tamMin)){
		tamMin = tamAtual;
		inicioMin = inicioAtual;
	}
	
	
	if (tamMin < ua) {
		return {ERROR_VALUE, 0};
	}

	tabelaParticao[pid] = {inicioMin, ua};
	int fim = MMU::limiteFisico(tabelaParticao[pid]);
	for (int i = inicioMin; i <= fim; i++){
		memoria[i] = {.free= false, .id = pid};
	}

	return {inicioMin, fim};
};

tuple<int, int> WorstFit::aloca(int pid, int ua) {
	int inicioAtual = 0, inicioMax = 0;
	int tamAtual = 0, tamMax = 0;
	for (int i = 0; i < TAM_MEMORIA; i++) {
		if (memoria[i].free){
			tamAtual++;
		} else{
			if (tamAtual >= tamMax) {
				tamMax = tamAtual;
				inicioMax = inicioAtual;
			}
			tamAtual = 0;
			inicioAtual = i + 1;
		} 
	}
	
	if (tamAtual >= tamMax) {
		tamMax = tamAtual;
		inicioMax = inicioAtual;
	}
	
	
	if (tamMax < ua) {
		return {ERROR_VALUE, 0};
	}

	tabelaParticao[pid] = {inicioMax, ua};
	int fim = MMU::limiteFisico(tabelaParticao[pid]);
	for (int i = inicioMax; i <= fim; i++){
		memoria[i] = {.free= false, .id = pid};
	}

	return {inicioMax, fim};
};

tuple<int, int> Buddy::aloca(int pid, int ua) {
	int tamNecessario = 1;
	while (tamNecessario < ua) tamNecessario<<=1;
	auto it = buddyMemo.lower_bound(tamNecessario);
	
	if (it == buddyMemo.end()) {
		return {ERROR_VALUE, 0};
	}

	int tam = it->first;
	int inicio = it->second;

	buddyMemo.erase(it);
	while (tam > tamNecessario) {	
		tam>>=1;
		buddyMemo.insert({tam, tam+inicio});
	}
	
	tabelaParticao[pid] = {inicio, tam};
	int fim = MMU::limiteFisico(tabelaParticao[pid]);
	
	return {inicio, fim};
};


tuple<int, int> Buddy::libera(int pid) {
	Particao p = tabelaParticao[pid];
	int inicio = p.base;
	int fim = MMU::limiteFisico(tabelaParticao[pid]);
	cout << p.base << " " << p.limite << " " << fim;
	for (int i = inicio; i <= fim; i++) {
		memoria[i] = {.free = true, .id = 0};
	}
	tabelaParticao.erase(pid);
	
	int tam = p.limite;
	int init = p.base;
	auto it = buddyMemo.lower_bound(p.limite);
	while (it!=buddyMemo.end() && it->first == tam){
		if((init ^ tam) == it->second){
			if (it->second < init){
				init = it->second;
			}
			tam<<=1;
			buddyMemo.erase(it);
			it = buddyMemo.lower_bound(tam);
		} else {
			it++;
		}
	}

	buddyMemo.insert({tam, inicio});

	return {inicio, fim};
};

void setIdOperacao() {
	idOperacao["aloca"] = OP_ALLOC;
	idOperacao["acessa"] = OP_ACCESS;
	idOperacao["libera"] = OP_FREE;
}



unordered_map<string, int> leIdProcessos(ifstream &arquivoEntrada, vector<string> & nomeProcesso){
	int qntdProcessos;
	arquivoEntrada >> qntdProcessos;
	unordered_map<string, int> idProcesso;
	string pid;
	
	getline(arquivoEntrada, pid); // terminar de ler a primeira linha

	for (int i = 0; i < qntdProcessos-1; i++){
		getline(arquivoEntrada, pid, ';');
		idProcesso[pid] = i;
		nomeProcesso.push_back(pid);
	}
	
	getline(arquivoEntrada, pid); // ultimo processo nao tem ';'
	idProcesso[pid] = qntdProcessos-1;
	nomeProcesso.push_back(pid);
	
	return idProcesso;
}

vector<Requisicao> leRequisicoes(ifstream &arquivoEntrada, unordered_map<string, int> & idProcesso){
	setIdOperacao();
	
	vector<Requisicao> requisicoes;

	unsigned int opCode;
	int param;
	Requisicao requisicao;

	string line, operacao, pid;
	while(getline(arquivoEntrada, line)){
		stringstream lineStream(line);
		lineStream >> operacao >> pid;
		
		opCode = idOperacao[operacao];
		if (opCode == OP_FREE) {
			requisicao = {opCode, idProcesso[pid], optional<int>()};
		} else {
			lineStream >> param;
			requisicao = {opCode, idProcesso[pid], optional<int>(param)};
		}

		requisicoes.push_back(requisicao);
	}

	return requisicoes;
}

string nomeLogFile(char* argv[]) {
	auto estrategiaAloc = string(argv[1]);
	size_t ultimoUnderline = estrategiaAloc.find_last_of("_");
	if (ultimoUnderline != string::npos)
		estrategiaAloc = estrategiaAloc.substr(0, ultimoUnderline);
	transform(estrategiaAloc.begin(), estrategiaAloc.end(), estrategiaAloc.begin(), ::tolower);


	auto arquivoEntrada = string(argv[2]);
	auto ultimaBarra = arquivoEntrada.find_last_of("/");
	string caminho = "";
	
	if (ultimaBarra != string::npos) {
		caminho = arquivoEntrada.substr(0, ultimaBarra+1);
		arquivoEntrada = arquivoEntrada.substr(ultimaBarra+1, string::npos);
	}

	size_t ultimoPonto = arquivoEntrada.find_last_of(".");
    if (ultimoPonto != string::npos)
		arquivoEntrada = arquivoEntrada.substr(0, ultimoPonto); 

	string nomeLog = "log_" + arquivoEntrada + "_" + estrategiaAloc + ".txt";

	return caminho + nomeLog;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		finalizar_com_erro("Quantidade incorreta de argumentos. Uso: ./simulador <estratégia> <arquivo_entrada>");
	}

	Estrategia estrategia;
	if (string(argv[1]) == "FIRST_FIT"){
		estrategia = FIRST_FIT;
	} else if (string(argv[1]) == "BEST_FIT") {
		estrategia = BEST_FIT;
	} else if (string(argv[1]) == "WORST_FIT") {
		estrategia = WORST_FIT;
	} else if (string(argv[1]) == "BUDDY") {
		estrategia = BUDDY;
	} else {
		return finalizar_com_erro("Estratégia de alocação inválida. Escolha entre FIRST_FIT - BEST_FIT - WORST_FIT - BUDDY");
	}
	
	ifstream arquivoEntrada(argv[2]);
	if (!arquivoEntrada.is_open()) {
		finalizar_com_erro("Arquivo de entrada não encontrado.");
	}

	idProcesso = leIdProcessos(arquivoEntrada, nomeProcesso);
	auto requisicoes = leRequisicoes(arquivoEntrada, idProcesso);

	arquivoEntrada.close();

	string nomeLog = nomeLogFile(argv);

	ofstream log(nomeLog);
	if (!log.is_open()){
		finalizar_com_erro("Arquivo de Log nao abriu");
	}

	Simulador simulador(log, estrategia);
	simulador.simular(requisicoes);
	
	log.close();
	cout << "tudo ok" <<endl;

	return 0;
}
