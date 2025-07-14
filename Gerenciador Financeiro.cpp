#include <iostream>
#define _WINSOCKAPI_ 
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <windows.h>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <thread>
#include <filesystem>
#include <sstream>
#include <conio.h>
#include <condition_variable>
#include <atomic>
#include <libpq-fe.h>
#define _CRT_SECURE_NO_WARNINGS

using namespace std;
mutex monitor_mutex;
condition_variable monitor_cv;
atomic<bool> monitorando{ true };  

//DADOS

struct Usuario {
    string nome;
    string senha;
    int permicao;
    int codigo;
    int servicosRealizados;
};

struct Despesas {
    string nome, valor, nome_escrevente, data;
    int codigo;
};

struct Pix {
    string nome, valor, nome_escrevente, data;
    int codigo;
};

struct Adiantados {
    string nome, valor, nome_escrevente, data;
    int codigo;
};

// FUNÇÕES

void writeAt(int x, int y, const string& text) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;

    
    COORD coord = { (SHORT)x, (SHORT)y };

    
    WriteConsoleOutputCharacterA(hConsole, text.c_str(), text.length(), coord, &written);
}

void gotoxy(int x, int y) {
    COORD coord;
    coord.X = x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

void limparAreaDespesas() {
    for (int j = 1; j < 41; j++) {
        for (int i = 0; i < 43; i++) {
            writeAt(i, j, " ");
        }
    }
}

void limparAreaPixs() {
    for (int j = 1; j < 41; j++) {
        for (int i = 88; i < 145; i++) {
            writeAt(i, j, " ");
        }
    }
}

string obterDataAtual() {
    time_t agora = time(0);
    struct tm tempoLocal;
    localtime_s(&tempoLocal, &agora); 

    char buffer[80];
    strftime(buffer, 80, "%d/%m/%Y", &tempoLocal);
    return string(buffer);
}

string Minusculo(string text) {
    for (size_t i = 0; i < text.size(); i++) {
        text[i] = tolower(text[i]);
    }
    return text;
}

string Maiusculo(string text) {
    for (size_t i = 0; i < text.size(); i++) {
        text[i] = toupper(text[i]);
    }
    return text;
}

string LerSenhaOculta() {
    string senha;
    char ch;

    while ((ch = _getch()) != '\r') { 
        if (ch == '\b') { 
            if (!senha.empty()) {
                senha.pop_back();      
                cout << "\b \b";       
            }
        }
        else {
            senha += ch;              
            cout << "*";              
        }
    }

    cout << endl;
    return senha;
}

string capturarTecla() {
    char tecla;
    string text;
    while (true) {
        tecla = _getch();

        if (tecla == 13) { 
            break;
        }
        else if (tecla == 27) {
            return "";
        }
        else if (tecla == 8) { 
            if (!text.empty()) {
                text.pop_back();
                cout << "\b \b";
            }
        }
        else if (tecla == 0 || tecla == -32) {
            _getch();
        }
        else {
            text += tecla;
            cout << tecla;
        }
    }
    return text;
}

// QUANTIDADES

int QuantidadeAtual(PGconn* conn, Usuario escrevente[8], int codigo) {
    if (conn == NULL || PQstatus(conn) != CONNECTION_OK) {
        cerr << "Conexão com o banco de dados não está OK: " << PQerrorMessage(conn) << endl;
        return 0;
    }

    if (codigo < 0 || codigo >= 8) {
        cerr << "Código de escrevente inválido!" << endl;
        return 0;
    }

    string data = obterDataAtual(); 

    string query =
        "SELECT "
        "(SELECT COUNT(*) FROM pix WHERE nome_escrevente = '" + escrevente[codigo].nome + "' AND data_pix = '" + data + "') AS total_pix, "
        "(SELECT COUNT(*) FROM despesa WHERE nome_escrevente = '" + escrevente[codigo].nome + "' AND data_despesa = '" + data + "') AS total_despesa;";

    PGresult* res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cerr << "Erro na consulta QUANTIDADE: " << PQerrorMessage(conn) << endl;
        PQclear(res);
        return 0;
    }

    int totalPix = 0;
    int totalDespesa = 0;
    int nrows = PQntuples(res);

    if (nrows > 0) {
        char* pixCountStr = PQgetvalue(res, 0, 0);      
        char* despesaCountStr = PQgetvalue(res, 0, 1); 
        totalPix = atoi(pixCountStr);
        totalDespesa = atoi(despesaCountStr);
    }
    else {
        cerr << "Consulta retornou 0 linhas." << endl;
    }

    PQclear(res);
    return totalPix + totalDespesa; 
}

int QuantidadeAtualAdiantado(PGconn* conn, Usuario escrevente[8], int codigo) {
    if (conn == NULL || PQstatus(conn) != CONNECTION_OK) {
        cerr << "Conexão com o banco de dados não está OK: " << PQerrorMessage(conn) << std::endl;
        return 0;
    }

    if (codigo < 0 || codigo >= 8) {
        std::cerr << "Código de escrevente inválido!" << std::endl;
        return 0;
    }

    string data = obterDataAtual();

    string query = "SELECT COUNT(*) FROM adiantado";

    PGresult* res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Erro na consulta QUANTIDADE: " << PQerrorMessage(conn) << std::endl;
        PQclear(res);
        return 0;
    }
    int nrows = PQntuples(res);

    int totalAdiantado = 0;

    if (nrows > 0) {
        char* countStr = PQgetvalue(res, 0, 0);
        totalAdiantado = atoi(countStr);
    }
    else {
        std::cerr << "Consulta retornou 0 linhas." << std::endl;
    }

    PQclear(res);
    return totalAdiantado;
}

// PIX BANCO

void CarregarPixAnteriorBanco(PGconn* conn, vector<Pix>& pix, Usuario escrevente[8], int codigo, string data) {
    string sql = "SELECT nome_cliente, valor, nome_escrevente, data_pix FROM pix WHERE data_pix = '" + data + "';";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cerr << "Erro ao consultar pixs: " << PQerrorMessage(conn) << endl;
        PQclear(res);
        return;
    }
    int nrows = PQntuples(res);

    for (int i = 0; i < nrows; ++i) {
        Pix p;
        p.nome = PQgetvalue(res, i, 0);
        p.valor = PQgetvalue(res, i, 1);
        p.nome_escrevente = PQgetvalue(res, i, 2);
        p.data = PQgetvalue(res, i, 3);
        pix.push_back(p);
    }
    PQclear(res);
}

void CarregarDespesaAnteriorBanco(PGconn* conn, vector<Despesas>& despesa, Usuario escrevente[8], int codigo, string data) {
    string sql = "SELECT nome_despesa, valor, nome_escrevente, data_despesa FROM despesa WHERE data_despesa = '" + data + "';";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cerr << "Erro ao consultar pixs: " << PQerrorMessage(conn) << endl;
        PQclear(res);
        return;
    }
    int nrows = PQntuples(res);

    for (int i = 0; i < nrows; ++i) {
        Despesas d;
        d.nome = PQgetvalue(res, i, 0);
        d.valor = PQgetvalue(res, i, 1);
        d.nome_escrevente = PQgetvalue(res, i, 2);
        d.data = PQgetvalue(res, i, 3);
        despesa.push_back(d);
    }
    PQclear(res);
}

void CarregarPixBanco(PGconn* conn, vector<Pix>& pix, Usuario escrevente[8], int codigo) {
    string data = obterDataAtual();
    pix.clear();

    string sql = "SELECT nome_cliente, valor, nome_escrevente, data_pix FROM pix WHERE data_pix = '" + data + "';";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cerr << "Erro ao consultar pixs: " << PQerrorMessage(conn) << endl;
        PQclear(res);
        return;
    }
    int nrows = PQntuples(res);
    
    for (int i = 0; i < nrows; ++i) {
        Pix p;
        p.nome = PQgetvalue(res, i, 0);
        p.valor = PQgetvalue(res, i, 1);
        p.nome_escrevente = PQgetvalue(res, i, 2);
        p.data = PQgetvalue(res, i, 3);
        pix.push_back(p);
    }
    PQclear(res);
}

void AdicionarPixBanco(PGconn* conn, vector<Pix>& pix, Usuario escrevente[8], int codigo) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }

    system("cls");
    Sleep(100);
    string data = obterDataAtual();
    int j = 0;
    string nome, valor;

    writeAt(46, 0, "Pixs Realizados: ");
    for (size_t i = 0; i < pix.size(); i++) {
        if (pix[i].data == data) {
            writeAt(46, j + 2, "R$ " + pix[i].valor + " - " + pix[i].nome + " - " + pix[i].nome_escrevente);
            j++; 
        }
    }
    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    ADICIONAR PIX";
    cout << "\n\n==========================================\n\n";
    cout << "Nome do cliente: ";
    nome = capturarTecla();
    if (nome == "") {
        return;
    }
    cout << "\nValor: ";
    valor = capturarTecla();
    if (valor == "") {
        return;
    }

    string sql = "INSERT INTO pix (nome_cliente, valor, nome_escrevente, data_pix) VALUES ('" + nome + "', '" + valor + "', '" + escrevente[codigo].nome + "', '" + data + "')";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
    }
    else {
        cout << "\nPIX inserido com sucesso!" << endl;
        escrevente[codigo].servicosRealizados++;
    }
    CarregarPixBanco(conn, pix, escrevente, codigo);
    PQclear(res);

    Sleep(1000);
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all(); 
}

void RemoverPixBanco(PGconn* conn, vector<Pix>& pix, Usuario escrevente[8], int codigo) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }

    system("cls");
    Sleep(100);
    string data = obterDataAtual();
    int j = 0;
    string nome;
    bool encontrou = false;

    
    {
        std::lock_guard<std::mutex> lock(monitor_mutex); 
        writeAt(46, 0, "Pix Realizados:");
        for (size_t i = 0; i < pix.size(); i++) {
            if (pix[i].data == data) {
                writeAt(46, j + 2, "R$ " + pix[i].valor + " - " + pix[i].nome + " - " + pix[i].nome_escrevente);
                cout << "\n";
                ++j;
            }
        }
    }

    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    REMOVER PIX";
    cout << "\n\n==========================================\n\n";
    cout << "Nome do cliente: ";
    nome = capturarTecla();
    if (nome == "") {
        return;
    }


    string sql = "DELETE FROM pix WHERE nome_cliente = '" + nome + "' AND nome_escrevente = '" + escrevente[codigo].nome + "';";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
    }
    else {
        std::lock_guard<std::mutex> lock(monitor_mutex); 
        for (auto it = pix.begin(); it != pix.end(); ) {
            if (it->nome == nome && it->data == data && it->nome_escrevente == escrevente[codigo].nome) {
                it = pix.erase(it);
                cout << "\nPix encontrado e removido\n";
                escrevente[codigo].servicosRealizados--;
                encontrou = true;
                Sleep(1000);
                break;
            }
            else {
                ++it;
            }
        }

        if (!encontrou) {
            cout << "\nCliente nao encontrado\n";
            Sleep(1000);
        }
    }

    PQclear(res);

    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all(); 
}

void EditarPixBanco(PGconn* conn, vector<Pix>& pix, Usuario escrevente[8], int codigo) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }

    system("cls");
    Sleep(100);
    string data = obterDataAtual();
    char tecla;
    int j = 0, escolha;
    string nome, novoNome, novoValor;
    bool encontrou = false;

    writeAt(46, 0, "Pixs Realizados:");
    for (size_t i = 0; i < pix.size(); i++) {
        if (pix[i].data == data) {
            writeAt(46, j + 2, "R$ " + pix[i].valor + " - " + pix[i].nome + " - " + pix[i].nome_escrevente);
            cout << "\n";
            ++j;
        }
    }

    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    EDITAR PIX";
    cout << "\n\n==========================================\n\n";
    cout << "O que deseja fazer:\n1- Alterar nome\n2- Alterar valor\n3- Ambos\n4- Voltar\nEscolha: ";
    cin >> escolha;

    switch (escolha) {
    case 1:
        system("cls");
        Sleep(100);
        writeAt(46, 0, "Pixs Realizados:");
        j = 0;
        for (size_t i = 0; i < pix.size(); i++) {
            if (pix[i].data == data) {
                writeAt(46, j + 2, "R$ " + pix[i].valor + " - " + pix[i].nome + " - " + pix[i].nome_escrevente);
                cout << "\n";
                ++j;
            }
        }
        gotoxy(0, 0);
        cout << "==========================================\n\n";
        cout << "    EDITAR PIX";
        cout << "\n\n==========================================\n\n";
        cout << "Nome do cliente: ";
        nome = capturarTecla();
        if (nome == "") {
            return;
        }
        for (size_t i = 0; i < pix.size(); i++) {
            if (pix[i].nome == nome) {
                cout << "\nNovo nome: ";
                novoNome = capturarTecla();
                if (novoNome == "") {
                    return;
                }
                pix[i].nome = novoNome;
                string sql = "UPDATE pix SET nome_cliente = '" + novoNome + "' WHERE nome_cliente = '" + nome + "';";
                PGresult* res = PQexec(conn, sql.c_str());
                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
                }
                else {
                    cout << "\nPix editado com sucesso!";
                    encontrou = true;
                }
                PQclear(res);
                Sleep(1000);
            }
        }
        if (!encontrou) {
            cout << "\nCliente nao encontrado!";
            Sleep(1000);
        }
        break;

    case 2:
        system("cls");
        Sleep(100);
        writeAt(46, 0, "Pixs Realizados:");
        j = 0;
        for (size_t i = 0; i < pix.size(); i++) {
            if (pix[i].data == data) {
                writeAt(46, j + 2, "R$ " + pix[i].valor + " - " + pix[i].nome + " - " + pix[i].nome_escrevente);
                cout << "\n";
                ++j;
            }
        }
        gotoxy(0, 0);
        cout << "==========================================\n\n";
        cout << "    EDITAR PIX";
        cout << "\n\n==========================================\n\n";
        cout << "Nome do cliente: ";
        nome = capturarTecla();
        if (nome == "") {
            return;
        }
        for (size_t i = 0; i < pix.size(); i++) {
            if (pix[i].nome == nome) {
                cout << "\nNovo valor: ";
                novoValor = capturarTecla();
                if (novoValor == "") {
                    return;
                }
                pix[i].valor = novoValor;
                string sql = "UPDATE pix SET valor = '" + novoValor + "' WHERE nome_cliente = '" + nome + "';";
                PGresult* res = PQexec(conn, sql.c_str());
                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
                }
                else {
                    cout << "\nPix editado com sucesso!";
                    encontrou = true;
                }
                PQclear(res);
                Sleep(1000);
            }
        }
        if (!encontrou) {
            cout << "\nCliente nao encontrado!";
            Sleep(1000);
        }
        break;

    case 3:
        system("cls");
        Sleep(100);
        writeAt(46, 0, "Pixs Realizados:");
        j = 0;
        for (size_t i = 0; i < pix.size(); i++) {
            if (pix[i].data == data) {
                writeAt(46, j + 2, "R$ " + pix[i].valor + " - " + pix[i].nome + " - " + pix[i].nome_escrevente);
                cout << "\n";
                ++j;
            }
        }
        gotoxy(0, 0);
        cout << "==========================================\n\n";
        cout << "    EDITAR PIX";
        cout << "\n\n==========================================\n\n";
        cout << "Nome do cliente: ";
        nome = capturarTecla();
        if (nome == "") {
            return;
        }

        for (size_t i = 0; i < pix.size(); i++) {
            if (pix[i].nome == nome) {
                cout << "\nNovo nome: ";
                novoNome = capturarTecla();
                if (novoNome == "") {
                    return;
                }
                cout << "\nNovo valor: ";
                novoValor = capturarTecla();
                if (novoValor == "") {
                    return;
                }

                
                pix[i].nome = novoNome;
                pix[i].valor = novoValor;

                
                string sql = "UPDATE pix SET nome_cliente = '" + novoNome + "', valor = '" + novoValor + "' WHERE nome_cliente = '" + nome + "';";
                PGresult* res = PQexec(conn, sql.c_str());

                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
                }
                else {
                    cout << "\nPix editado com sucesso!";
                    encontrou = true;
                }

                PQclear(res);
                Sleep(1000);
                break;
            }
        }
        if (!encontrou) {
            cout << "\nCliente nao encontrado!";
            Sleep(1000);
        }
        break;
    }
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all(); 
}

// DESPESA BANCO

void CarregarDespesaBanco(PGconn* conn, vector<Despesas>& despesa, Usuario escrevente[8], int codigo) {
    string data = obterDataAtual();

    string sql = "SELECT nome_despesa, valor, nome_escrevente, data_despesa FROM despesa WHERE data_despesa = '" + data + "';";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cerr << "Erro ao consultar despesas: " << PQerrorMessage(conn) << endl;
        PQclear(res);
        return;
    }
    int nrows = PQntuples(res);

    for (int i = 0; i < nrows; ++i) {
        Despesas d;
        d.nome = PQgetvalue(res, i, 0);
        d.valor = PQgetvalue(res, i, 1);
        d.nome_escrevente = PQgetvalue(res, i, 2);
        d.data = PQgetvalue(res, i, 3);
        despesa.push_back(d);
    }
    PQclear(res);
}

void AdicionarDespesaBanco(PGconn* conn, vector<Despesas>& despesa, Usuario escrevente[8], int codigo) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }

    system("cls");
    Sleep(100);
    string data = obterDataAtual();
    char tecla;
    int j = 0;
    string nome, valor;

    writeAt(46, 0, "Despesas Realizadas:");
    for (size_t i = 0; i < despesa.size(); i++) {
        if (despesa[i].data == data) {
            writeAt(46, j + 2, "R$ " + despesa[i].valor + " - " + despesa[i].nome + " - " + despesa[i].nome_escrevente);
            cout << "\n";
        }
    }
    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    ADICIONAR DESPESA";
    cout << "\n\n==========================================\n\n";
    cout << "Nome da despesa: ";
    nome = capturarTecla();
    if (nome == "") {
        return;
    }
    cout << "\nValor: ";
    valor = capturarTecla();
    if (valor == "") {
        return;
    }

    string sql = "INSERT INTO despesa (nome_despesa, valor, nome_escrevente, data_despesa) VALUES ('" + nome + "', '" + valor + "', '" + escrevente[codigo].nome + "', '" + data + "')";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
    }
    else {
        cout << "\nDESPESA inserida com sucesso!" << endl;
        escrevente[codigo].servicosRealizados++;
    }
    CarregarDespesaBanco(conn, despesa, escrevente, codigo);
    PQclear(res);
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all();
    Sleep(1000);
}

void RemoverDespesaBanco(PGconn* conn, vector<Despesas>& despesa, Usuario escrevente[8], int codigo) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }
    system("cls");
    Sleep(100);
    string data = obterDataAtual();
    char tecla;
    int j = 0;
    string nome;
    bool encontrou = false;

    writeAt(46, 0, "Despesas Realizadas:");
    for (size_t i = 0; i < despesa.size(); i++) {
        if (despesa[i].data == data) {
            writeAt(46, j + 2, "R$ " + despesa[i].valor + " - " + despesa[i].nome + " - " + despesa[i].nome_escrevente);
            cout << "\n";
            ++j;
        }
    }

    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    REMOVER DESPESA";
    cout << "\n\n==========================================\n\n";
    cout << "Nome da despesa: ";
    nome = capturarTecla();
    if (nome == "") {
        return;
    }

    string sql = "DELETE FROM despesa WHERE nome_despesa = '" + nome + "' AND nome_escrevente = '" + escrevente[codigo].nome + "';";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
    }
    else {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        for (auto it = despesa.begin(); it != despesa.end(); ) {
            if (it->nome == nome && it->data == data && it->nome_escrevente == escrevente[codigo].nome) {
                it = despesa.erase(it);
                cout << "\nDespesa encontrada e removida\n";
                escrevente[codigo].servicosRealizados--;
                encontrou = true;
                Sleep(1000);
                break;
            }
            else {
                ++it;
            }
        }

        if (!encontrou) {
            cout << "\nDespesa nao encontrada\n";
            Sleep(1000);
        }
    }

    PQclear(res);

    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all();
}

void EditarDespesaBanco(PGconn* conn, vector<Despesas>& despesa, Usuario escrevente[8], int codigo) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }

    system("cls");
    Sleep(100);
    string data = obterDataAtual();
    char tecla;
    int j = 0, escolha;
    string nome, novoNome, novoValor;
    bool encontrou = false;

    writeAt(46, 0, "Despesas Realizadas:");
    for (size_t i = 0; i < despesa.size(); i++) {
        if (despesa[i].data == data) {
            writeAt(46, j + 2, "R$ " + despesa[i].valor + " - " + despesa[i].nome + " - " + despesa[i].nome_escrevente);
            cout << "\n";
            ++j;
        }
    }

    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    EDITAR DESPESA";
    cout << "\n\n==========================================\n\n";
    cout << "O que deseja fazer:\n1- Alterar nome\n2- Alterar valor\n3- Ambos\n4- Voltar\n Escolha: ";
    cin >> escolha;

    switch (escolha) {
    case 1:
        system("cls");
        Sleep(100);
        writeAt(46, 0, "Despesas Realizadas:");
        j = 0;
        for (size_t i = 0; i < despesa.size(); i++) {
            if (despesa[i].data == data) {
                writeAt(46, j + 2, "R$ " + despesa[i].valor + " - " + despesa[i].nome + " - " + despesa[i].nome_escrevente);
                cout << "\n";
                ++j;
            }
        }
        gotoxy(0, 0);
        cout << "==========================================\n\n";
        cout << "    EDITAR DESPESA";
        cout << "\n\n==========================================\n\n";
        cout << "Nome da despesa: ";
        nome = capturarTecla();
        if (nome == "") {
            return;
        }
        for (size_t i = 0; i < despesa.size(); i++) {
            if (despesa[i].nome == nome) {
                cout << "\nNovo nome: ";
                novoNome = capturarTecla();
                if (novoNome == "") {
                    return;
                }
                despesa[i].nome = novoNome;
                string sql = "UPDATE despesa SET nome_despesa = '" + novoNome + "' WHERE nome_despesa = '" + nome + "';";
                PGresult* res = PQexec(conn, sql.c_str());
                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
                }
                else {
                    cout << "\nDespesa editada com sucesso!";
                    encontrou = true;
                }
                PQclear(res);
                Sleep(1000);
            }
        }
        if (!encontrou) {
            cout << "\nDespesa nao encontrada!";
            Sleep(1000);
        }
        break;

    case 2:
        system("cls");
        Sleep(100);
        writeAt(46, 0, "Despesas Realizadas:");
        j = 0;
        for (size_t i = 0; i < despesa.size(); i++) {
            if (despesa[i].data == data) {
                writeAt(46, j + 2, "R$ " + despesa[i].valor + " - " + despesa[i].nome + " - " + despesa[i].nome_escrevente);
                cout << "\n";
                ++j;
            }
        }
        gotoxy(0, 0);
        cout << "==========================================\n\n";
        cout << "    EDITAR DESPESA";
        cout << "\n\n==========================================\n\n";
        cout << "Nome da despesa: ";
        nome = capturarTecla();
        if (nome == "") {
            return;
        }
        for (size_t i = 0; i < despesa.size(); i++) {
            if (despesa[i].nome == nome) {
                cout << "\nNovo valor: ";
                novoValor = capturarTecla();
                if (novoValor == "") {
                    return;
                }
                despesa[i].valor = novoValor;
                string sql = "UPDATE despesa SET valor = '" + novoValor + "' WHERE nome_despesa = '" + nome + "';";
                PGresult* res = PQexec(conn, sql.c_str());
                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
                }
                else {
                    cout << "\nDespesa editada com sucesso!";
                    encontrou = true;
                }
                PQclear(res);
                Sleep(1000);
            }
        }
        if (!encontrou) {
            cout << "\nDespesa nao encontrada!";
            Sleep(1000);
        }
        break;

    case 3:
        system("cls");
        Sleep(100);
        writeAt(46, 0, "Despesas Realizadas:");
        j = 0;
        for (size_t i = 0; i < despesa.size(); i++) {
            if (despesa[i].data == data) {
                writeAt(46, j + 2, "R$ " + despesa[i].valor + " - " + despesa[i].nome + " - " + despesa[i].nome_escrevente);
                cout << "\n";
                ++j;
            }
        }
        gotoxy(0, 0);
        cout << "==========================================\n\n";
        cout << "    EDITAR DESPESA";
        cout << "\n\n==========================================\n\n";
        cout << "Nome da despesa: ";
        nome = capturarTecla();
        if (nome == "") {
            return;
        }

        for (size_t i = 0; i < despesa.size(); i++) {
            if (despesa[i].nome == nome) {
                cout << "\nNovo nome: ";
                novoNome = capturarTecla();
                if (novoNome == "") {
                    return;
                }
                cout << "\nNovo valor: ";
                novoValor = capturarTecla();
                if (novoValor == "") {
                    return;
                }

               
                despesa[i].nome = novoNome;
                despesa[i].valor = novoValor;

                
                string sql = "UPDATE despesa SET nome_despesa = '" + novoNome + "', valor = '" + novoValor + "' WHERE nome_despesa = '" + nome + "';";
                PGresult* res = PQexec(conn, sql.c_str());

                if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                    cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
                }
                else {
                    cout << "\nDespesa editada com sucesso!";
                    encontrou = true;
                }

                PQclear(res);
                Sleep(1000);
                break;
            }
        }

        if (!encontrou) {
            cout << "\nDespesa nao encontrada!";
            Sleep(1000);
        }
        break;
    }
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all();
}

// PAGAMENTOS ADIANTADOS

float ConverterValores(string valor) {
    float valorConvertido = 0;
    replace(valor.begin(), valor.end(), ',', '.');
    valorConvertido = stof(valor);
    return valorConvertido;
}

void CarregarAdiantadoBanco(PGconn* conn, vector<Adiantados>& adiantado, Usuario escrevente[8], int codigo) {
    string data = obterDataAtual();
    adiantado.clear();

    string sql = "SELECT nome_adiantado, valor, nome_escrevente, data_adiantado FROM adiantado;";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        cerr << "Erro ao consultar despesas: " << PQerrorMessage(conn) << endl;
        PQclear(res);
        return;
    }
    int nrows = PQntuples(res);

    for (int i = 0; i < nrows; ++i) {
        Adiantados a;
        a.nome = PQgetvalue(res, i, 0);
        a.valor = PQgetvalue(res, i, 1);
        a.nome_escrevente = PQgetvalue(res, i, 2);
        a.data = PQgetvalue(res, i, 3);
        adiantado.push_back(a);
    }
    PQclear(res);
}

void AdicionarPagamentoAdiantado(PGconn* conn, vector<Adiantados>& adiantado, Usuario escrevente[8], int codigo, int &qntAdiantado) {
    string nome, valor, data,dataSub;
    int j = 0;
    data = obterDataAtual();
    dataSub = data.substr(0, 5);
    system("cls");
    Sleep(100);
    writeAt(46, 0, "Servicos Pendentes:");
    for (size_t i = 0; i < adiantado.size(); i++) {
        writeAt(46, j + 2, "R$ " + adiantado[i].valor + " - " + adiantado[i].nome + " - " + adiantado[i].nome_escrevente);
        j++;
    }
    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    ADICIONAR PAGAMENTO";
    cout << "\n\n==========================================\n\n";
    cout << "Nome: ";
    nome = capturarTecla();
    if (nome == "") {
        return;
    }
    cout << "\nValor: ";
    valor = capturarTecla();
    if (valor == "") {
        return;
    }

    string sql = "INSERT INTO adiantado (nome_adiantado, valor, nome_escrevente, data_adiantado) VALUES ('" + nome + " " + dataSub + "', '" + valor + "', '" + escrevente[codigo].nome + "', '" + data + "')";
    PGresult* res = PQexec(conn, sql.c_str());

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
        Sleep(5000);
    }
    else {
        cout << "\nPAGAMENTO inserido com sucesso!" << endl;
        qntAdiantado++;
        Sleep(1000);
    }
    CarregarAdiantadoBanco(conn, adiantado, escrevente, codigo);
    PQclear(res);
}

void RealizarBaixa(PGconn* conn, vector<Adiantados>& adiantado, Usuario escrevente[8], int codigo, vector<Pix>& pix, int &qntAdiantado) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }

    string nome;
    string data = obterDataAtual();
    int j = 0;

    if (adiantado.empty()) {
        system("cls");
        Sleep(100);
        cout << "Nenhum servico registrado!";
        Sleep(1000);
        return;
    }

    system("cls");
    writeAt(46, 0, "Servicos Pendentes:");
    for (size_t i = 0; i < adiantado.size(); i++) {
        writeAt(46, j + 2, "R$ " + adiantado[i].valor + " - " + adiantado[i].nome + " - " + adiantado[i].nome_escrevente);
        j++;
    }

    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    REALIZAR BAIXA";
    cout << "\n\n==========================================\n\n";
    cout << "Nome: ";
    nome = capturarTecla();
    if (nome == "") {
        return;
    }

    for (size_t i = 0; i < adiantado.size(); i++) {
        if (adiantado[i].nome == nome) {
            
            PQexec(conn, "BEGIN");

            
            string insert_sql = "INSERT INTO pix (nome_cliente, valor, nome_escrevente, data_pix) VALUES ('" +
                adiantado[i].nome + "', '" + adiantado[i].valor + "', '" + adiantado[i].nome_escrevente + "', '" + data + "')";
            PGresult* res = PQexec(conn, insert_sql.c_str());

            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                cerr << "Erro ao inserir no pix: " << PQerrorMessage(conn) << endl;
                Sleep(5000);
                PQexec(conn, "ROLLBACK");
                PQclear(res);
                return;
            }
            PQclear(res);

            
            string delete_sql = "DELETE FROM adiantado WHERE nome_adiantado = '" + nome + "'";
            res = PQexec(conn, delete_sql.c_str());

            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                cerr << "Erro ao remover de adiantado: " << PQerrorMessage(conn) << endl;
                PQexec(conn, "ROLLBACK");
                Sleep(5000);
                PQclear(res);
                return;
            }
            PQclear(res);

          
            PQexec(conn, "COMMIT");

            cout << "\nPIX inserido e adiantado removido com sucesso!" << endl;
            qntAdiantado--;
            escrevente[codigo].servicosRealizados++;
            CarregarPixBanco(conn, pix, escrevente, codigo);
            CarregarAdiantadoBanco(conn, adiantado, escrevente, codigo);
            break; 
        }
    }
    Sleep(1000);
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all();
}

void RemoverPagamento(PGconn* conn, vector<Adiantados>& adiantado, Usuario escrevente[8], int codigo, int& qntAdiantado){
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }

    if (adiantado.empty()) {
        system("cls");
        cout << "Nenhum servico registrado!";
        Sleep(1000);
        return;
    }

    string nome;
    string data = obterDataAtual();
    int j = 0;

    system("cls");
    Sleep(100);
    writeAt(46, 0, "Servicos Pendentes:");
    for (size_t i = 0; i < adiantado.size(); i++) {
        writeAt(46, j + 2, "R$ " + adiantado[i].valor + " - " + adiantado[i].nome + " - " + adiantado[i].nome_escrevente);
        j++;
    }

    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    REMOVER PAGAMENTO";
    cout << "\n\n==========================================\n\n";
    cout << "Nome: ";
    nome = capturarTecla();
    if (nome == "") {
        return;
    }

    for (size_t i = 0; i < adiantado.size(); i++) {
        if (adiantado[i].nome == nome) {
            cout << "\nPagamento encontrado!";
            string sql = "DELETE FROM adiantado WHERE nome_adiantado = '" + nome + "'";
            PGresult* res = PQexec(conn, sql.c_str());

            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                cerr << "Erro ao executar comando: " << PQerrorMessage(conn) << endl;
                Sleep(5000);
            }
            else {
                CarregarAdiantadoBanco(conn, adiantado, escrevente, codigo);
                cout << "\nPagamento removido!";
                qntAdiantado--;
                PQclear(res);
            }
        }
    }
    Sleep(1000);
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all();
}

void BaixaParcial(PGconn* conn, vector<Adiantados>& adiantado, Usuario escrevente[8], int codigo, vector<Pix>& pix) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }

    string nome, valorUser, valorFinalString;
    float valorConvertido, valorFinal;
    string data = obterDataAtual();
    bool negativo = true;
    int j = 0;

    if (adiantado.empty()) {
        system("cls");
        Sleep(100);
        cout << "Nenhum servico registrado!";
        Sleep(1000);
        return;
    }
    system("cls");
    writeAt(46, 0, "Servicos Pendentes:");
    for (size_t i = 0; i < adiantado.size(); i++) {
        writeAt(46, j + 2, "R$ " + adiantado[i].valor + " - " + adiantado[i].nome + " - " + adiantado[i].nome_escrevente);
        j++;
    }

    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "       REALIZAR BAIXA PARCIAL";
    cout << "\n\n==========================================\n\n";
    cout << "Nome: ";
    nome = capturarTecla();
    if (nome == "") {
        return;
    }
    cout << "\nValor que deseja dar baixa: ";
    valorUser = capturarTecla();
    if (valorUser == "") {
        return;
    }
    float valorUserConvertido = ConverterValores(valorUser);

    for (size_t i = 0; i < adiantado.size(); i++) {
        if (adiantado[i].nome == nome) {
            valorConvertido = ConverterValores(adiantado[i].valor);
            while (negativo) {
                valorFinal = valorConvertido - valorUserConvertido;
                if (valorFinal > 0) {
                    negativo = false;
                }
                else {
                    cout << "\nValor maior que o esperado!\n";
                    cout << "Valor: ";
                    valorUser = capturarTecla();
                    if (valorUser == "") {
                        return;
                    }
                    valorUserConvertido = ConverterValores(valorUser);
                }
            }

            std::ostringstream stream;
            stream << std::fixed << std::setprecision(2) << valorFinal;
            valorFinalString = stream.str();
            replace(valorFinalString.begin(), valorFinalString.end(), '.', ',');
            PQexec(conn, "BEGIN");

           
            string insert_sql = "INSERT INTO pix (nome_cliente, valor, nome_escrevente, data_pix) VALUES ('" +
                adiantado[i].nome + "', '" + valorUser + "', '" + adiantado[i].nome_escrevente + "', '" + data + "')";
            PGresult* res = PQexec(conn, insert_sql.c_str());

            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                cerr << "Erro ao inserir no pix: " << PQerrorMessage(conn) << endl;
                Sleep(5000);
                PQexec(conn, "ROLLBACK");
                PQclear(res);
                return;
            }
            PQclear(res);

            string delete_sql = "UPDATE adiantado SET valor = '" + valorFinalString + "' WHERE nome_adiantado = '" + nome + "'";
            res = PQexec(conn, delete_sql.c_str());

            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                cerr << "Erro ao atualizar de adiantado: " << PQerrorMessage(conn) << endl;
                PQexec(conn, "ROLLBACK");
                Sleep(5000);
                PQclear(res);
                return;
            }
            PQclear(res);

          
            PQexec(conn, "COMMIT");

            cout << "\nPIX inserido e adiantado atualizado com sucesso!" << endl;
            CarregarPixBanco(conn, pix, escrevente, codigo);
            CarregarAdiantadoBanco(conn, adiantado, escrevente, codigo);
            break; 
        }
    }
    Sleep(1000);
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all();
}

void PagamentosAdiantados(PGconn* conn, vector<Adiantados>& adiantado, Usuario escrevente[8], int codigo, vector<Pix>& pix, int &qntAdiantado) {
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }
    int escolha;
    string data = obterDataAtual();
    int j = 0;
    system("cls");
    Sleep(100);
    writeAt(46, 0, "Servicos Pendentes:");
    for (size_t i = 0; i < adiantado.size(); i++) {
        writeAt(46, j + 2, "R$ " + adiantado[i].valor + " - " + adiantado[i].nome + " - " + adiantado[i].nome_escrevente);
        j++; 
    }
    gotoxy(0, 0);
    cout << "==========================================\n\n";
    cout << "    PAGAMENTOS EM HAVER";
    cout << "\n\n==========================================\n\n";
    cout << "1- Adicionar Pagamento\n2- Realizar Baixa\n3- Remover Pagamento\n4- Baixa Parcial\n5- Voltar\nEscolha: ";
    cin >> escolha;

    switch (escolha) {
    case 1:
        AdicionarPagamentoAdiantado(conn, adiantado, escrevente, codigo, qntAdiantado);
        break;
    case 2:
        RealizarBaixa(conn, adiantado, escrevente, codigo, pix, qntAdiantado);
        break;
    case 3:
        RemoverPagamento(conn, adiantado, escrevente, codigo, qntAdiantado);
        break;
    case 4:
        BaixaParcial(conn, adiantado, escrevente, codigo, pix);
        break;
    case 5:
        break;
    }
    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all();
}

// DATAS ANTERIORES


// EXIBIÇÃO

void exibirPixs(const vector<Pix>& pix) {
    string dataAtual = obterDataAtual();
    writeAt(88, 0, "Pixs Realizados:");

    int j = 2;
    for (size_t i = 0; i < pix.size(); ++i) {
        if (pix[i].data == dataAtual) {
            writeAt(88, j, "R$ " + pix[i].valor + " - " + pix[i].nome + " - " + pix[i].nome_escrevente);
            j++;
        }
    }
    gotoxy(54, 19);
}

void exibirDespesas(const vector<Despesas>& despesa) {
    string dataAtual = obterDataAtual();
    writeAt(1, 0, "Despesas Realizadas:");

    int j = 2;
    for (size_t i = 0; i < despesa.size(); ++i) {
        if (despesa[i].data == dataAtual) {
            writeAt(1, j, "R$ " + despesa[i].valor + " - " + despesa[i].nome + " - " + despesa[i].nome_escrevente);
            j++;
        }
    }
    gotoxy(54, 20);
}

// MENUS E MAIS

void AtualizarMenu() {
    return;
}

void exibirMenuCentral(PGconn* conn, Usuario escrevente[8], int codigo, string user, int qntAdiantado) {
    time_t agora = time(0);
    tm tempoLocal;
    localtime_s(&tempoLocal, &agora); 

    string texto = "                  " + user + "(" + std::to_string(escrevente[codigo].servicosRealizados) + ")";
    string adiantados = "9- Pagamentos Adiantados(" + to_string(qntAdiantado) + ")";

    writeAt(44, 0, "==========================================");
    writeAt(44, 2, "    SEJA BEM-VINDO AO CONTROLE DO PIX");
    gotoxy(44, 3);
    cout << "                " << setfill('0') << setw(2) << tempoLocal.tm_mday << "/" << setw(2) << tempoLocal.tm_mon + 1 << "/" << tempoLocal.tm_year + 1900 << ".";
    writeAt(44, 4, texto);
    writeAt(44, 6, "==========================================");
    writeAt(44, 8, "1- Adicionar Pix");
    writeAt(44, 9, "2- Remover Pix");
    writeAt(44, 10, "3- Editar");
    writeAt(44, 11, "4- Adicionar Despesa");
    writeAt(44, 12, "5- Remover Despesa");
    writeAt(44, 13, "6- Editar Despesa");
    writeAt(44, 14, "7- Conferir por data");
    writeAt(44, 15, "8- Atualizar menu");
    writeAt(44, 16, adiantados);
    writeAt(44, 17, "10- Sair");
    writeAt(44, 19, "Escolha: ");
    gotoxy(53, 19);
    Sleep(200);
}

void MonitorarMudancasBanco(PGconn* conn, vector<Pix>& pix, vector<Despesas>& despesa, vector<Adiantados>& adiantado, Usuario escrevente[8], int codigo, int &qntAdiantado, string user) {
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "Erro ao inicializar Winsock." << endl;
        return;
    }

    PQexec(conn, "LISTEN canal_pix;");
    PQexec(conn, "LISTEN canal_despesa;");
    PQexec(conn, "LISTEN canal_adiantado;");

    while (true) {
        {
            std::unique_lock<std::mutex> lock(monitor_mutex);
            monitor_cv.wait(lock, [] { return monitorando.load(); });
        }

        int sock = PQsocket(conn);
        if (sock < 0) break;

        fd_set input_mask;
        FD_ZERO(&input_mask);
        FD_SET(sock, &input_mask);

        timeval timeout;
        timeout.tv_sec = 2;  
        timeout.tv_usec = 0;

        int ret = select(sock + 1, &input_mask, NULL, NULL, &timeout);
        if (ret == SOCKET_ERROR) {
            cerr << "Erro no select(): " << WSAGetLastError() << endl;
            break;
        }

        
        PQconsumeInput(conn);

        if (ret > 0 && FD_ISSET(sock, &input_mask)) {
            PGnotify* notify;
            while ((notify = PQnotifies(conn)) != nullptr) {
                string canal = notify->relname;

                {
                    std::lock_guard<std::mutex> lock(monitor_mutex);
                    if (canal == "canal_pix") {
                        pix.clear();
                        CarregarPixBanco(conn, pix, escrevente, codigo);
                        limparAreaPixs();
                        exibirPixs(pix);
                    }
                    else if (canal == "canal_despesa") {
                        despesa.clear();
                        CarregarDespesaBanco(conn, despesa, escrevente, codigo);
                        limparAreaDespesas();
                        exibirDespesas(despesa);
                    }
                    else if (canal == "canal_adiantado") {
                        adiantado.clear();
                        qntAdiantado = QuantidadeAtualAdiantado(conn, escrevente, codigo);
                        exibirMenuCentral(conn, escrevente, codigo, user, qntAdiantado);
                        CarregarAdiantadoBanco(conn, adiantado, escrevente, codigo);
                    }
                }

                PQfreemem(notify);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    WSACleanup();
}

void CriarImpressao(vector<Pix> pix, vector<Despesas> despesa, string data) {
    char* userProfile = nullptr;
    size_t len = 0;
    _dupenv_s(&userProfile, &len, "USERPROFILE");

    if (userProfile) {
        string caminho = string(userProfile) + "\\Desktop\\Impressao.txt";
        ofstream arquivo(caminho);
        free(userProfile);

        if (arquivo.is_open()) {
            arquivo << "Pix realizados na data: " << data << "\n";

            for (size_t i = 0; i < pix.size(); i++) {
                arquivo << "R$ " << pix[i].valor << " - " << pix[i].nome << " - " << pix[i].nome_escrevente << "\n";
            }

            arquivo << "===============================\n";

            arquivo << "\nDespesas realizadas na data: " << data << "\n";
            for (size_t i = 0; i < despesa.size(); i++) {
                arquivo << "R$ " << despesa[i].valor << " - " << despesa[i].nome << " - " << despesa[i].nome_escrevente << "\n";
            }

            cout << "\nArquivos salvos com sucesso!";
        }
    }
}

void ConferirData(PGconn* conn, Usuario escrevente[8], int codigo) {
    {

        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = false;
    }

    system("cls");
    Sleep(100);
    vector<Pix> pix;
    vector<Despesas> despesa;
    vector<Pix> dataPix;

    vector<Despesas> dataDespesa;
    string data;
    string resposta;
    char tecla;
    int j = 0;
    cout << "==========================================\n\n";
    cout << "    CONFERIR PIX";
    cout << "\n\n==========================================\n\n";
    cout << "Qual data voce deseja? 00/00/0000\n";
    cout << "Data: ";
    while (true) {
        tecla = _getch();
        if (tecla == 13) {
            break;
        }
        else if (tecla == 27) {
            return;
        }
        else if (tecla == 8) {
            if (!data.empty()) {
                data.pop_back();
                cout << "\b \b";
            }
        }
        else {
            data += tecla;
            cout << tecla;
        }
    }

    CarregarPixAnteriorBanco(conn, pix, escrevente, codigo, data);
    CarregarDespesaAnteriorBanco(conn, despesa, escrevente, codigo, data);

    bool encontrou = false;
    cout << "\n\nPixs desta data: \n\n";
    for (size_t i = 0; i < pix.size(); i++) {
        if (pix[i].data == data) {
            cout << j + 1 << ") R$ " << pix[i].valor << " - " << pix[i].nome << " - " << pix[i].nome_escrevente << "\n";
            dataPix.push_back(pix[i]);
            encontrou = true;
            j++;
        }
    }

    if (!encontrou) {
        cout << "\nNenhum Pix encontrado para essa data.\n";
    }

    cout << "\n================================";

    cout << "\n\nDespesas desta data: \n\n";
    j = 0;
    encontrou = false;  
    for (size_t i = 0; i < despesa.size(); i++) {
        if (despesa[i].data == data) {
            cout << j + 1 << ") R$ " << despesa[i].valor << " - " << despesa[i].nome << " - " << despesa[i].nome_escrevente << "\n";
            dataDespesa.push_back(despesa[i]);
            encontrou = true;
            j++;
        }
    }

    if (!encontrou) {
        cout << "\nNenhuma despesa encontrada para essa data.\n";
    }
    cout << "\nCriar versao para impressao? S/N\n";
    cin >> resposta;

    if (resposta == "S" || resposta == "s") {
        CriarImpressao(dataPix, dataDespesa, data);
    }

    cout << "\nVoltando para o menu!";
    Sleep(1000);

    {
        std::lock_guard<std::mutex> lock(monitor_mutex);
        monitorando = true;
    }
    monitor_cv.notify_all();
}

int main() {
    system("mode con: cols=145 lines=41");
    
    HWND consoleWindow = GetConsoleWindow();
    
    LONG style = GetWindowLong(consoleWindow, GWL_STYLE);
    style &= ~(WS_MAXIMIZEBOX | WS_SIZEBOX); 
    SetWindowLong(consoleWindow, GWL_STYLE, style);

    wstring titulo = L"Gerenciador de Pix - Tabelionato de Notas e Protestos - Viradouro/SP";
    SetConsoleTitleW(titulo.c_str());

    PGconn* conn = PQconnectdb("host= port=5432 dbname= user= password=");

    if (PQstatus(conn) != CONNECTION_OK) {
        system("cls");
        Sleep(100);
        cerr << "Erro na conexão com banco: " << PQerrorMessage(conn) << endl;
        Sleep(5000);
        return 1;
    }

    string user, senha;
    Usuario usuario[8];
    bool correto = false;
    int codigo;

    usuario[0] = { "Augusto", "", 0, 0 };
    usuario[1] = { "Andressa", "", 1, 1 };
    usuario[2] = { "Gilberto", "", 1, 2 };
    usuario[3] = { "Roberta", "", 0, 3 };
    usuario[4] = { "Maria", "", 0, 4 };
    usuario[5] = { "Paulo", "", 0, 5 };
    usuario[6] = { "Helena", "", 0, 6 };
    usuario[7] = { "Diogo", "", 0, 7 };

    writeAt(39, 0, "=======================Login========================");
    writeAt(48, 2, "SEJA BEM-VINDO AO CONTROLE DO PIX");
    writeAt(39, 4, "==========Tabelionato de Notas e Protestos==========");

    while (!correto) {
        gotoxy(48, 6);
        writeAt(39, 6, "Usuario: ");
        cin >> user;

        gotoxy(46, 7);
        writeAt(39, 7, "Senha: ");
        senha = LerSenhaOculta();

        user = Minusculo(user);
        senha = Minusculo(senha);

        for (int i = 0; i < 8; i++) {
            if (Minusculo(usuario[i].nome) == user && Minusculo(usuario[i].senha) == senha) {
                gotoxy(39, 9);
                writeAt(39, 9, "                                                  ");
                cout << "Acesso concedido! Ola " << usuario[i].nome << "!\n";
                codigo = usuario[i].codigo;
                Sleep(1000);
                correto = true;
                break;
            }
        }
        if (!correto) {
            gotoxy(39, 9);
            cout << "Usuario ou senha incorretos digite novamente!\n\n";
            writeAt(47, 6, "                                                                                      ");
            writeAt(39, 7, "                                                                                      ");
        }
    }
    user = Maiusculo(user);

    vector<Despesas> despesa;
    CarregarDespesaBanco(conn, despesa, usuario, codigo);

    vector<Pix> pix;
    CarregarPixBanco(conn, pix, usuario, codigo);

    vector<Adiantados> adiantado;
    CarregarAdiantadoBanco(conn, adiantado, usuario, codigo);

    usuario[codigo].servicosRealizados = QuantidadeAtual(conn, usuario, codigo);
    int adiantadoRealizados = QuantidadeAtualAdiantado(conn, usuario, codigo);

    thread monitor_thread(MonitorarMudancasBanco, conn, ref(pix), ref(despesa), ref(adiantado), usuario, codigo, ref(adiantadoRealizados), user);
    monitor_thread.detach();

    int escolha = 0;
    while (escolha != 10) {
        system("cls");
        Sleep(100);
        exibirDespesas(despesa);
        exibirPixs(pix);
        exibirMenuCentral(conn, usuario, codigo, user, adiantadoRealizados);
        cin >> escolha;

        switch (escolha) {
        case 1:
            AdicionarPixBanco(conn, pix, usuario, codigo);
            break;
        case 2:
            RemoverPixBanco(conn, pix, usuario, codigo);
            break;
        case 3:
            EditarPixBanco(conn, pix, usuario, codigo);
            break;
        case 4:
            AdicionarDespesaBanco(conn, despesa, usuario, codigo);
            break;
        case 5:
            RemoverDespesaBanco(conn, despesa, usuario, codigo);
            break;
        case 6:
            EditarDespesaBanco(conn, despesa, usuario, codigo);
            break;
        case 7:
            ConferirData(conn, usuario, codigo);
            break;
        case 8:
            AtualizarMenu();
            break;
        case 9:
            PagamentosAdiantados(conn, adiantado, usuario, codigo, pix, adiantadoRealizados);
            break;
        case 10:
            gotoxy(44, 20);
            cout << "Saindo . . .";
            Sleep(1000);
            return 0;
        default:
            cout << "Escolha invalida!\n";
            break;
        }
    }
}