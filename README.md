# 💼 Gerenciador Financeiro

![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=c%2B%2B&logoColor=white)
![PostgreSQL](https://img.shields.io/badge/PostgreSQL-336791?style=flat&logo=postgresql&logoColor=white)

Desenvolvido totalmente em C++, este é um gerenciador financeiro criado para uma aplicação real no **Tabelionato de Notas e Protestos - Viradouro/SP**.

---

## 🖼️ Interface (Terminal)

O sistema roda via terminal do Windows, com interface limpa e responsiva:
- Exibição em tela dividida de Pix e Despesas do dia
- Menu centralizado com ações rápidas
- Interface baseada em coordenadas (`gotoxy`, `writeAt`)

---

## ⚙️ Funcionalidades

- ✅ Registro e exibição de Pix e Despesas do dia  
- ✅ Baixa total ou parcial de pagamentos adiantados  
- ✅ Visualização e impressão de registros por data  
- ✅ Integração com PostgreSQL usando `LISTEN/NOTIFY` para atualização em tempo real  
- ✅ Controle de usuários com permissões e autenticação  

---

## 🛠️ Tecnologias Utilizadas

- 💻 **C++**
- 🐘 **PostgreSQL**
- 🧵 **Threading** com `std::thread`, `mutex`, `condition_variable`
- 📟 API do **Windows Console**
- 📦 `libpq` (PostgreSQL C API)

---

## 🔐 Login de Usuários

- Controle de acesso com 8 usuários predefinidos  
- Entrada de senha oculta no console  
- Registro de serviços realizados por cada escrevente  

---

## 📈 Melhorias Futuras

- [ ] Tabela para cadastramento de novos usuários  
- [ ] Banco de dados otimizado com mais tabelas e relacionamentos  
- [ ] Interface gráfica (GUI)  
- [ ] Proteção contra SQL Injection  

---

## ⚠️ Observações

- Este projeto utiliza **PostgreSQL** com três tabelas principais e três canais `LISTEN/NOTIFY`.  
- Substitua a linha **1647** no código pelo seu string de conexão com o banco.  
- Adicione uma senha entre aspas (`""`) após o nome dos usuários na linha **1662** para permitir o login.  

### 🗃️ Estrutura das Tabelas:

```sql
CREATE TABLE pix (
  nome_cliente TEXT,
  valor TEXT,
  nome_escrevente TEXT,
  data_pix TEXT
);

CREATE TABLE despesa (
  nome_despesa TEXT,
  valor TEXT,
  nome_escrevente TEXT,
  data_despesa TEXT
);

CREATE TABLE adiantado (
  nome_adiantado TEXT,
  valor TEXT,
  nome_escrevente TEXT,
  data_adiantado TEXT
);
```

### 🔔 Canais para `LISTEN/NOTIFY`:

```sql
NOTIFY canal_pix;
NOTIFY canal_despesa;
NOTIFY canal_adiantado;
```

---

## 💻 Autor

**João Filipe Betin**  
🔗 [GitHub](https://github.com/JoaoBetin)

---

## 📄 Licença de Uso

Este projeto foi desenvolvido para uso interno e **não está autorizado qualquer tipo de redistribuição** sem a devida autorização do autor.

---

## 🙏 Agradecimentos

- Agradeço ao **Cartório de Viradouro** pela confiança no meu projeto e por utilizá-lo no dia a dia.

#

## 📙 English Version

# 💼 Financial Manager

![C++](https://img.shields.io/badge/C++-00599C?style=flat&logo=c%2B%2B&logoColor=white)
![PostgreSQL](https://img.shields.io/badge/PostgreSQL-336791?style=flat&logo=postgresql&logoColor=white)

Fully developed in C++, this is a financial management system created for a real-world application at the **Notary and Protest Office of Viradouro/SP – Brazil**.

---

## 🖼️ Terminal Interface

The system runs on the Windows terminal with a clean and responsive interface:
- Split-screen display of Pix and Expenses of the day
- Centralized menu with quick actions
- Coordinate-based interface using (`gotoxy`, `writeAt`)

---

## ⚙️ Features

- ✅ Register and view Pix and Expenses of the day  
- ✅ Full or partial clearance of advance payments  
- ✅ View and print records by date  
- ✅ PostgreSQL integration with `LISTEN/NOTIFY` for real-time updates  
- ✅ User login system with permissions and authentication  

---

## 🛠️ Technologies Used

- 💻 **C++**
- 🐘 **PostgreSQL**
- 🧵 **Threading** with `std::thread`, `mutex`, `condition_variable`
- 📟 **Windows Console API**
- 📦 `libpq` (PostgreSQL C API)

---

## 🔐 User Login

- Access control with 8 predefined users  
- Password input hidden in the terminal  
- Tracks number of services performed per user  

---

## 📈 Future Improvements

- [ ] User registration table  
- [ ] More optimized database with relational structure  
- [ ] Graphical User Interface (GUI)  
- [ ] SQL Injection protection  

---

## ⚠️ Notes

- This project uses **PostgreSQL** with three main tables and three `LISTEN/NOTIFY` channels.  
- Replace line **1647** in the code with your own database connection string.  
- Add a password inside the empty quotes (`""`) for each user on line **1662** to enable login.  

### 🗃️ Table Structure:

```sql
CREATE TABLE pix (
  nome_cliente TEXT,
  valor TEXT,
  nome_escrevente TEXT,
  data_pix TEXT
);

CREATE TABLE despesa (
  nome_despesa TEXT,
  valor TEXT,
  nome_escrevente TEXT,
  data_despesa TEXT
);

CREATE TABLE adiantado (
  nome_adiantado TEXT,
  valor TEXT,
  nome_escrevente TEXT,
  data_adiantado TEXT
);
```

### 🔔 `LISTEN/NOTIFY` Channels:

```sql
NOTIFY canal_pix;
NOTIFY canal_despesa;
NOTIFY canal_adiantado;
```

---

## 💻 Author

**João Filipe Betin**  
🔗 [GitHub](https://github.com/JoaoBetin)

---

## 📄 Usage License

This project was developed for internal use and **redistribution is not authorized** without the author's explicit permission.

---

## 🙏 Acknowledgements

- Special thanks to the **Viradouro Notary Office** for trusting and using this project in their daily routine.
