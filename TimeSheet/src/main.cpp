#include <iostream>
#include <fstream>  //lo usamos para los archivos de texto
#include <string> 
#include <vector> 
#include <ctime> //lo usamos para las fechas y horas  
#include <cctype> //lo usamos para tolower
#include <cstdint> //para uint32_t y uint8_t
#include <sstream> //para convertir a string en hexadecimal
#include <iomanip> //para setw y setfill en el hex
using namespace std; 

//es la ruta para los archivos del usuario
const string USERS_FILE = "data/users.txt";
//es la ruta para los archivos de ponche
const string TIMESHEETS_FILE = "data/timesheets.txt";
//es la ruta para el historial
const string LOG_FILE = "data/log.txt";
//la estructura que utilizamos para guardar los datos de los timesheets
struct Timesheet {
    int id;
    int ownerId;
    string date;
    int hours;
    string status;
    string task;
};
//son las variables que guardan la informacion del usuario que esta logueado ahora mismo
int currentUserId = -1;
string currentUsername = "";
string currentUserRole = "";
bool isLoggedIn = false;

//va a devolver true si el texto tiene espacio
    bool tieneEspacios(string texto) {
    return texto.find(' ') != string::npos;
    }
//para verificar que el string que utilizamos sea un numero entero
bool esEnteroPositivo(const string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

//lo usamos para convertir el string en numero
int convertirEntero(const string& s) {
    int valor = 0;
    for (char c : s) {
        valor = valor * 10 + (c - '0');
    }
    return valor;
}

bool fechaValida(const string& fecha) {
    //El formato que vamos a utilizar para la fecha
    if (fecha.size() != 10) return false;

    //Las posiciones 4 y 7  de la fecha pueden ser solamente '-'
    if (fecha[4] != '-' || fecha[7] != '-') return false;

    //Lo demas tienen que ser numeros
    int indices[] = {0,1,2,3,5,6,8,9};
    for (int i : indices) {
        if (!isdigit(static_cast<unsigned char>(fecha[i]))) {
            return false;
        }
    }

    //aqui extraemos los numeros de cada posicion
    string yearStr  = fecha.substr(0, 4);
    string monthStr = fecha.substr(5, 2);
    string dayStr   = fecha.substr(8, 2);

    //Verificamos si son numeros enteros
    if (!esEnteroPositivo(monthStr) || !esEnteroPositivo(dayStr)) {
        return false;
    }
    //los convertimos a int
    int month = convertirEntero(monthStr);
    int day   = convertirEntero(dayStr);

    //Verificamos si el mes es desde el 1 al 12
    if (month < 1 || month > 12) {
        return false;
    }

    //Verificamos que el dia sea desde el 1 al 31
    if (day < 1 || day > 31) {
        return false;
    }

    return true;
}
//aqui sacamos la fecha y hora actual en formato YYYY-MM-DD_HH:MM:SS
string obtenerTimestamp() {
    time_t now = time(nullptr);
    tm* local = localtime(&now);

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H:%M:%S", local);

    return string(buffer);
}
//Funcion que vamos a utilizar para hash los passwords de usuario
uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}
string sha256(const string& input) {
    //constantes de SHA-256 source: chatgpt
    static const uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };

    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef372;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x510e527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;

    vector<uint8_t> data(input.begin(), input.end());
    uint64_t bitLen = static_cast<uint64_t>(data.size()) * 8;

    data.push_back(0x80);
    while (data.size() % 64 != 56) {
        data.push_back(0x00);
    }

    for (int i = 7; i >= 0; --i) {
        data.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xff));
    }

    for (size_t chunk = 0; chunk < data.size(); chunk += 64) {
        uint32_t w[64];

        for (int i = 0; i < 16; ++i) {
            size_t j = chunk + i * 4;
            w[i] = (static_cast<uint32_t>(data[j]) << 24) |
                   (static_cast<uint32_t>(data[j + 1]) << 16) |
                   (static_cast<uint32_t>(data[j + 2]) << 8) |
                   (static_cast<uint32_t>(data[j + 3]));
        }

        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;
        uint32_t f = h5;
        uint32_t g = h6;
        uint32_t h = h7;

        for (int i = 0; i < 64; ++i) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = h + S1 + ch + k[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
        h5 += f;
        h6 += g;
        h7 += h;
    }

    stringstream ss;
    ss << hex << setfill('0');
    ss << setw(8) << h0
       << setw(8) << h1
       << setw(8) << h2
       << setw(8) << h3
       << setw(8) << h4
       << setw(8) << h5
       << setw(8) << h6
       << setw(8) << h7;

    return ss.str();
}

//La funcion que usaremos en el proyecto ya convertida
string hashPassword(const string& password) {
    return sha256(password);
}
//funcion donde se va a guardar el log
void logAction(int userId,const string& role,const string& action,const string& entity,int entityId,const string& status,const string& message) {
    ofstream logFile(LOG_FILE, ios::app);
    if (!logFile.is_open()) {
        return;
    }

    string ts = obtenerTimestamp();
//formato en el que se va a mostrar adentro del log
    logFile << ts << " ; "
            << userId << " ; "
            << role << " ; "
            << action << " ; "
            << entity << " ; "
            << entityId << " ; "
            << status << " ; "
            << message << "\n";

    logFile.close();
}

//para pedir datos y guardarlos en users.txt
void registrarUsuario(){
    string username;
    string password;
    string role;
    string roleInput; 

    if (cin.peek() == '\n') {
        cin.get();
    }

    cout << "=== Registro de Usuario ===\n\n";

    //pedimos el username y verificamos que no tenga espacio, que no este vacio y que no pase de 20 caracteres
    do {
        cout << "Username (no puede tener espacios y maximo 20 caracteres): ";
        getline(cin, username);

        if (username.empty()) { 
            cout << "El username no puede estar vacio.\n";
        }
        else if (tieneEspacios(username)) {
            cout << "El username no puede tener espacios.\n";
        }
        else if (username.size() > 20) { 
            cout << "El username no puede tener mas de 20 caracteres.\n";
        }

    } while (username.empty() || tieneEspacios(username) || username.size() > 20);

    //repetimos el mismo paso para el password
    do {
        cout << "Password (no puede tener espacios y maximo 20 caracteres): ";
        getline(cin, password);

        if (password.empty()) {
            cout << "El password no puede estar vacio.\n";
        }
        else if (tieneEspacios(password)) {
            cout << "El password no puede tener espacios.\n";
        }
        else if (password.size() > 20) {  
            cout << "El password no puede tener mas de 20 caracteres.\n";
        }

    } while (password.empty() || tieneEspacios(password) || password.size() > 20);

    //pedimos el rol que va a tomar el usuario
    bool rolValido = false;

    do {
        cout << "Rol Employee (e) o Supervisor (s): ";
        getline(cin, roleInput);

        //tiene que tener exactamente una letra que es la e o la s
        if (roleInput.size() != 1) {
            cout << "Solo se permite escribir una letra (e) o (s).\n";
            continue;
        }

        //aqui la convertimos en tolower y verificamos que letra entro el usuario y le da el rol que escogio
        char letter = tolower(roleInput[0]);

        if (letter == 'e') {
            role = "employee";
            rolValido = true;
        } else if (letter == 's') {
            role = "supervisor";
            rolValido = true;
        } else {
            cout << "Solo se permite la letra (e) o (s).\n"; //si no es ninguna de las dos opciones le tira mensaje de error
        }

    } while (!rolValido);

    //aqui buscamos el ultimo id usado en el archivo
    int lastId = 0;
    ifstream inFile(USERS_FILE);

    if (inFile.is_open()) {
        int fileId;
        string u, p, r;

        //cada usuario tiene id username password y role
        while (inFile >> fileId >> u >> p >> r) {
            if (fileId > lastId) {
                lastId = fileId;
            }
        }

        inFile.close();
    }

    //si el archivo no existe o esta vacio el lastId se queda en 0
    int newId = lastId + 1;

    //abrimos el archivo para agregar el usuario nuevo
    ofstream outFile(USERS_FILE, ios::app);

    //conertimos el hash del password
    string passwordHash = hashPassword(password);

    //cada usuario tiene id username password_hash y role
    outFile << newId << " "
            << username << " "
            << passwordHash << " "
            << role << "\n";

    outFile.close();

    cout << "\nUsuario registrado.\n";

    //actualizamos el log y orden en el que se van a registrar los usuarios
    logAction(newId,
            role,                  
            "REGISTER_USER",
            "user",
            newId,
            "success",
            "usuario registrado");

}

//funcion para ver los timesheets de todos como supervisor
void verTimesheetsEquipo(){
    if (!isLoggedIn || currentUserRole != "supervisor") { //si no es supervisor le tira un mensaje de error
        cout << "No tienes permiso para ver estos timesheets.\n";
        return;
    }

    //tira mensaje de eror si no se pudo abrir el archivo
    ifstream inFile(TIMESHEETS_FILE);
    if (!inFile.is_open()) {
        cout << "No se pudo abrir el archivo de timesheets.\n";
        return;
    }

    cout << "=== Timesheets del equipo ===\n\n";

    Timesheet t;
    bool hayAlguno = false;

    //Formato del timesheet
    while (inFile >> t.id >> t.ownerId >> t.date >> t.hours >> t.status >>t.task) {
        hayAlguno = true;
        cout << "ID: " << t.id
             << " | Owner ID: " << t.ownerId
             << " | Fecha: " << t.date
             << " | Horas: " << t.hours
             << " | Estado: " << t.status << " "
             << " | Tarea: " << t.task << "\n";
    }

    inFile.close();

    if (!hayAlguno) {
        cout << "No hay timesheets registrados.\n";
    }
}

//funcion para aprovar timesheets como supervisor
void aprobarTimesheet() {
    if (!isLoggedIn || currentUserRole != "supervisor") {
        cout << "No tienes permiso para aprobar timesheets.\n";
        return;
    }

    ifstream inFile(TIMESHEETS_FILE);
    if (!inFile.is_open()) {
        cout << "No se pudo abrir el archivo de timesheets.\n";
        return;
    }
    //usamos un vector para la lista del timesheet
    vector<Timesheet> lista;
    Timesheet t;

    //lo agregamos todo al vector
    while (inFile >> t.id >> t.ownerId >> t.date >> t.hours >> t.status >> t.task) {
        lista.push_back(t);
    }

    inFile.close();

    cout << "=== Aprobar Timesheet ===\n\n";

    bool haySubmitted = false;

    //mostramos solamente los timesheets que esten submitted por el empleado
    for (const auto& ts : lista) {
        if (ts.status == "submitted") {
            haySubmitted = true;
            cout << "ID: " << ts.id
                 << " | Owner ID: " << ts.ownerId
                 << " | Fecha: " << ts.date
                 << " | Horas: " << ts.hours
                 << " | Estado: " << ts.status << " "
                 << " | Tarea: " << ts.task << "\n";
        }
    }
    //si no hay ninguno submitted nos tira este mensaje de error
    if (!haySubmitted) {
        cout << "No hay timesheets en estado submitted para aprobar.\n";
        return;
    }
    //seccion para aprobar los timesheets ya submitted
    string idInput;
    int idAprobar;
    cout << "\nIntroduce el ID del timesheet que quieres aprobar: ";
    cin >> idInput;
    //verificamos si el ID que puso es un ID valido usando la funcion para verificar si el string es un numero correcto
    if (!esEnteroPositivo(idInput)) {
    cout << "Favor de escribir un ID valido.\n";
    return;
    }
    //convertimos el ID del timesheet elegido a su numero entero
    idAprobar = convertirEntero(idInput);
    //si el ID es correcto y esta submitted nos deja aprobar el timesheet
    bool encontrado = false;
    for (auto& ts : lista) {
        if (ts.id == idAprobar && ts.status == "submitted") {
            encontrado = true;
            ts.status = "approved";
            cout << "\nTimesheet #" << ts.id << " aprobado.\n";

            //le damos un comentario de aprobacion al timesheet
            string comentario;
            if (cin.peek() == '\n') {
                cin.get();
            }

            cout << "Comentario de aprobacion: ";
            getline(cin, comentario);
            //si no pone nada le tira un mensaje generico
            if (comentario.empty()) {
                comentario = "sin comentario";
            }
            //actualizamos el log diciendo que fue aprovado succesfully
        logAction(currentUserId,
                currentUserRole,   
                "APPROVE_TS",
                "timesheet",
                    ts.id,
                      "success",
                      comentario);

            break;
         }
         }
         //si no es encontrado te tira un mensaje de error
    if (!encontrado) {
        cout << "No se encontro un timesheet submitted con ese ID.\n";
        return;
    }

    //lo agregamos al archivo
    ofstream outFile(TIMESHEETS_FILE, ios::trunc);
    //guardamos los timesheets ya actualizados en el archivo
    for (const auto& ts : lista) {
        outFile << ts.id << " "
                << ts.ownerId << " "
                << ts.date << " "
                << ts.hours << " "
                << ts.status << " "
                << ts.task << "\n";
    }

    outFile.close();
}
//aqui es donde rechazamos los timesheets como supervisor (es la misma descripcion pero con la opcion de rechazar)
void rechazarTimesheet() {
    if (!isLoggedIn || currentUserRole != "supervisor") {
        cout << "No tienes permiso para rechazar timesheets.\n";
        return;
    }

    ifstream inFile(TIMESHEETS_FILE);
    if (!inFile.is_open()) {
        cout << "No se pudo abrir el archivo de timesheets.\n";
        return;
    }

    vector<Timesheet> lista;
    Timesheet t;

    while (inFile >> t.id >> t.ownerId >> t.date >> t.hours >> t.status >> t.task) {
        lista.push_back(t);
    }

    inFile.close();

    cout << "=== Rechazar Timesheet ===\n\n";

    bool haySubmitted = false;

    for (const auto& ts : lista) {
        if (ts.status == "submitted") {
            haySubmitted = true;
            cout << "ID: " << ts.id
                 << " | Owner ID: " << ts.ownerId
                 << " | Fecha: " << ts.date
                 << " | Horas: " << ts.hours
                 << " | Estado: " << ts.status << " "
                 << " | Tarea: " << ts.task << "\n";
        }
    }

    if (!haySubmitted) {
        cout << "No hay timesheets en estado submitted para rechazar.\n";
        return;
    }

    string idInput;
    int idRechazar;
    cout << "\nID del timesheet que quieres rechazar: ";
    cin >> idInput;

    if (!esEnteroPositivo(idInput)) {
    cout << "Escribir un ID valido.\n";
    return;
    }

    idRechazar = convertirEntero(idInput);


    bool encontrado = false;
    for (auto& ts : lista) {
        if (ts.id == idRechazar && ts.status == "submitted") {
            encontrado = true;
            ts.status = "rejected";
            cout << "\nTimesheet con ID " << ts.id << " rechazado (status = rejected).\n";

            string comentario;
            if (cin.peek() == '\n') {
                cin.get(); 
            }

            cout << "Comentario de rechazo: ";
            getline(cin, comentario);

            if (comentario.empty()) {
                comentario = "sin comentario";
            }

        logAction(currentUserId,
                      currentUserRole,
                      "REJECT_TS",
                      "timesheet",
                      ts.id,
                      "success",
                      comentario);

            break;
        }
    }

    if (!encontrado) {
        cout << "No se encontro un timesheet SUBMITTED con ese ID.\n";
        return;
    }

    ofstream outFile(TIMESHEETS_FILE, ios::trunc);

    for (const auto& ts : lista) {
        outFile << ts.id << " "
                << ts.ownerId << " "
                << ts.date << " "
                << ts.hours << " "
                << ts.status << " "
                << ts.task << "\n";
    }

    outFile.close();
}
//funcion para crear timesheets como empleado
void crearTimesheetDraft() {
     if (!isLoggedIn || currentUserRole != "employee") { //si no eres empleado te tira mensaje de error
        cout << "No tienes permiso para crear timesheets.\n";
        return;
    }

    int hours;
    string date;
    string hoursInput;
    string task;
    //le pedimos las horas que trabajo la fecha y que tarea tuvo durante el turno
    do {
        cout << "Horas trabajadas (1 a 24): ";
        cin >> hoursInput;

        if (!esEnteroPositivo(hoursInput)) {
            cout << "Debe ser un numero del 1 al 24.\n";
            continue;
        }

        hours = convertirEntero(hoursInput);

        if (hours <= 0 || hours > 24) {
            cout << "Las horas deben estar entre 1 y 24.\n";
        }

    } while (hours <= 0 || hours > 24);

    do {
        cout << "Fecha (yyyy-mm-dd): ";
        cin >> date;

        if (!fechaValida(date)) {
            cout << "La fecha debe tener el formato escrito.\n";
        }

    } while (!fechaValida(date));

    if (cin.peek() == '\n') {
    cin.get();
}

while (true) {
    cout << "Descripcion corta de la tarea (sin espacios y maximo 30 caracteres): ";
    getline(cin, task);

    if (task.empty()) {
        cout << "La tarea no puede estar vacia.\n";
        continue;
    }
    if (tieneEspacios(task)) {
        cout << "La tarea no puede tener espacios.\n";
        continue;
    }
    if (task.size() > 30) {
        cout << "La tarea no puede tener mas de 30 caracteres.\n";
        continue;
    }

    break; 
}
//buscamos el ultimo id en timesheets.txt
    int lastId = 0;
    ifstream inFile(TIMESHEETS_FILE);

    if (inFile.is_open()) {
        int fileId, ownerId, fileHours;
        string fileDate, fileStatus, fileTask;

        //el formato que vamos a ver en el file
        while (inFile >> fileId >> ownerId >> fileDate >> fileHours >> fileStatus >> fileTask) {
            if (fileId > lastId) {
                lastId = fileId;
            }
        }

        inFile.close();
    }

    int newId = lastId + 1;

    //abrimos el archivo para agregar el nuevo timesheet
    ofstream outFile(TIMESHEETS_FILE, ios::app);

    string status = "draft";

    //Guardamos en este formato
    outFile << newId << " "
            << currentUserId << " "
            << date << " "
            << hours << " "
            << status << " "
            << task << "\n";

    outFile.close();

    cout << "\nTimesheet creado con ID " << newId << "\n";
//actualizamos el log diciendo que se creo un timesheet en draft
    logAction(currentUserId,
        currentUserRole,        
        "CREATE_TS",
        "timesheet",
        newId,
        "success",
        "timesheet creado en draft");

}
//aqui editamos el timesheet como empleado
void editarTimesheetDraft(){
if (!isLoggedIn || currentUserRole != "employee") { //no puedes editar al menos que seas empleado
        cout << "No tienes permiso para editar timesheets.\n";
        return;
    }

    ifstream inFile(TIMESHEETS_FILE);
    if (!inFile.is_open()) {
        cout << "No se pudo abrir el archivo de timesheets.\n";
        return;
    }
    //creamos el vector para agregar todos los timesheets
    vector<Timesheet> lista;
    Timesheet t;
    while (inFile >> t.id >> t.ownerId >> t.date >> t.hours >> t.status >> t.task) {
        lista.push_back(t);
    }

    inFile.close();

    //solamente le mostramos al usuario los timesheets que estan en estado draft
    cout << "=== Editar Timesheet Drafts ===\n\n";
    bool hayDrafts = false;
    //si encuentra algun timesheet tipo draft nos muestra en este orden
    for (const auto& ts : lista) {
        if (ts.ownerId == currentUserId && ts.status == "draft") {
            hayDrafts = true;
            cout << "ID: " << ts.id
                 << " | Fecha: " << ts.date
                 << " | Horas: " << ts.hours
                 << " | Estado: " << ts.status << " "
                 << " | Tarea: " << ts.task << "\n";
        }
    }
//si el usuario no tiene timesheets en draft te tira un mensaje de error
    if (!hayDrafts) {
        cout << "No tienes timesheets en draft para editar.\n";
        return;
    }

    string idInput;
    int idEditar;

    cout << "\nIntroduce el ID del timesheet que quieres editar: ";
    cin >> idInput;

    if (!esEnteroPositivo(idInput)) {
        cout << "Debes escribir un ID valido.\n";
        return;
    }

idEditar = convertirEntero(idInput);
    //buscamos el timesheet para editar si encontramos un timesheet en estado draft lo muestra
    bool encontrado = false;
    for (auto& ts : lista) {
        if (ts.id == idEditar &&
            ts.ownerId == currentUserId &&
            ts.status == "draft") {

            encontrado = true;
        
        string nuevaFecha;
        string horasInput;
        int nuevasHoras;
        string nuevaTask;

        //aqui le vamos a poner la nueva fecha para editar
        do {
            cout << "Nueva fecha (yyyy-mm-dd): ";
            cin >> nuevaFecha;

            if (!fechaValida(nuevaFecha)) {
                cout << "La fecha debe tener el formato escrito\n";
            }

        } while (!fechaValida(nuevaFecha));

        //aqui le vamos a poner las nuevas horas para editar
        do {
            cout << "Nuevas horas trabajadas (1 a 24): ";
            cin >> horasInput;

            if (!esEnteroPositivo(horasInput)) {
                cout << "Debes escribir un numero entero positivo.\n";
                continue;
            }

            nuevasHoras = convertirEntero(horasInput);

            if (nuevasHoras <= 0 || nuevasHoras > 24) {
                cout << "Las horas deben estar entre 1 y 24.\n";
            }

        } while (nuevasHoras <= 0 || nuevasHoras > 24);

        //aqui le vamos a poner la nueva tarea para editar

        //para limpiar antes del getline
        if (cin.peek() == '\n') {
            cin.get();
        }
//le escribimos la nueva tarea
        while (true) {
            cout << "Nueva descripcion de la tarea (sin espacios y un maximo de 30 caracteres): ";
            getline(cin, nuevaTask);

            if (nuevaTask.empty()) {
                cout << "La tarea no puede estar vacia.\n";
                continue;
            }
            if (tieneEspacios(nuevaTask)) {
                cout << "La tarea no puede tener espacios.\n";
                continue;
            }
            if (nuevaTask.size() > 30) {
                cout << "La tarea no puede tener mas de 30 caracteres.\n";
                continue;
            }

            break;
        }

        //actualizamos el cambio que hicimos
        ts.date  = nuevaFecha;
        ts.hours = nuevasHoras;
        ts.task  = nuevaTask;

        cout << "\nTimesheet actualizado.\n";
//actualizamos el log con lo que va a mostrar
        logAction(currentUserId,
            currentUserRole,
            "EDIT_TS",
            "timesheet",
            ts.id,
            "success",
            "timesheet draft editado");
            break;
        }
    }

    if (!encontrado) {
        cout << "No se encontro un timesheet DRAFT con ese ID.\n";
        return;
    }

    //actualizamos el archivo con la nueva lista
    ofstream outFile(TIMESHEETS_FILE, ios::trunc);

    for (const auto& ts : lista) {
        outFile << ts.id << " "
                << ts.ownerId << " "
                << ts.date << " "
                << ts.hours << " "
                << ts.status << " "
                << ts.task << "\n";
    }

    outFile.close();
}
//para someter timesheets de draft como empleado
void enviarTimesheet(){
if (!isLoggedIn || currentUserRole != "employee") {
        cout << "No tienes permiso para enviar timesheets.\n";
        return;
    }

    ifstream inFile(TIMESHEETS_FILE);
    if (!inFile.is_open()) {
        cout << "No se pudo abrir el archivo de timesheets.\n";
        return;
    }

    vector<Timesheet> lista;
    Timesheet t;

    
    while (inFile >> t.id >> t.ownerId >> t.date >> t.hours >> t.status >> t.task) {
        lista.push_back(t);
    }

    inFile.close();

    cout << "=== Enviar Timesheet (submit)===\n\n";
    bool hayDrafts = false;

    //muestra los timesheets de draft
    for (const auto& ts : lista) {
        if (ts.ownerId == currentUserId && ts.status == "draft") {
            hayDrafts = true;
            cout << "ID: " << ts.id
                 << " | Fecha: " << ts.date
                 << " | Horas: " << ts.hours
                 << " | Estado: " << ts.status << " "
                 << " | Tarea: " << ts.task << "\n";
        }
    }
//de no haber ningun timesheet en estado draft nos tira un mensaje de error
    if (!hayDrafts) {
        cout << "No tienes timesheets en estado DRAFT para enviar.\n";
        return;
    }
    //de haber encontrado nos muestra los timesheets y nos deja elegir un ID para someter
    string idInput;
    int idEnviar;
    cout << "\nID del timesheet que quieres enviar: ";
    cin >> idInput;

    if (!esEnteroPositivo(idInput)) {
    cout << "Favor de escribir un ID valido.\n";
    return;
    }

    idEnviar = convertirEntero(idInput);
//si es draft cae como true y nos deja someter cambiandole el estado a submitted
    bool encontrado = false;
    for (auto& ts : lista) {
        if (ts.id == idEnviar &&
            ts.ownerId == currentUserId &&
            ts.status == "draft") {
            encontrado = true;
            ts.status = "submitted";
            cout << "\nTimesheet con ID " << ts.id << " enviado (status = submitted).\n";
//actualizamos el log
        logAction(currentUserId,
          currentUserRole,
          "SUBMIT_TS",
          "timesheet",
          ts.id,
          "success",
          "timesheet enviado (submitted)");
        break;
        }
    }
//si no encontramos timesheet en estado draft nos tira un mensaje de error
    if (!encontrado) {
        cout << "No se encontro un timesheet draft con ese ID.\n";
        return;
    }

    //actualizamos el archivo de timesheets
    ofstream outFile(TIMESHEETS_FILE, ios::trunc);

    for (const auto& ts : lista) {
        outFile << ts.id << " "
                << ts.ownerId << " "
                << ts.date << " "
                << ts.hours << " "
                << ts.status << " "
                << ts.task << "\n";
    }

    outFile.close();
}
//funcion para ver los timesheets como empleado
void verMisTimesheets(){
    if (!isLoggedIn || currentUserRole != "employee") {
        cout << "No tienes permiso para ver estos timesheets.\n";
        return;
    }

    ifstream inFile(TIMESHEETS_FILE);
    if (!inFile.is_open()) {
        cout << "No se pudo abrir el archivo de timesheets.\n";
        return;
    }

    cout << "=== Mis Timesheets ===\n\n";

    int fileId, ownerId, hours;
    string date, status, task;
    bool hayAlguno = false;

   
    while (inFile >> fileId >> ownerId >> date >> hours >> status >> task) {
        if (ownerId == currentUserId) {
            hayAlguno = true;
            cout << "ID: " << fileId
                 << " | Fecha: " << date
                 << " | Horas: " << hours
                 << " | Estado: " << status << " "
                 << " | Tarea: " << task << "\n";
        }
    }

    inFile.close();

    if (!hayAlguno) {
        cout << "No tienes timesheets registrados.\n";
    }
}

void menuEmployee() {
    int op = 0;
    string input;

    do {
        cout << "=== Menu Employee ===\n\n";
        cout << "1. Crear timesheet (draft)\n";
        cout << "2. Ver mis timesheets\n";
        cout << "3. Editar timesheet en draft\n";
        cout << "4. Enviar (submit) timesheet\n";
        cout << "5. Cerrar sesion\n\n";
        cout << "Opcion: ";

    
        if (cin.peek() == '\n') {
            cin.get();
        }

        //aqui leemos la opcion como string
        if (!getline(cin, input)) {
            //si hay algun error se sale del menu
            return;
        }

        //verificamos que sea solo un caracter y que sea entre el 1 y el 5 
        if (input.size() != 1 || input[0] < '1' || input[0] > '5') {
            cout << "\nDebes escribir un numero entre 1 y 5.\n\n";
            continue; // vuelve a mostrar el menu
        }

        //lo convertimos a int
        op = input[0] - '0';

        cout << "\n";
        //interface del menu
        switch (op) {
            case 1:
                crearTimesheetDraft();
                break;
            case 2:
                verMisTimesheets();
                break;
            case 3:
                editarTimesheetDraft();
                break;
            case 4:
                enviarTimesheet();
                break;
            case 5:
                cout << "Cerrando sesion de " << currentUsername << "...\n";
                isLoggedIn = false;
                currentUserId = -1;
                currentUsername = "";
                currentUserRole = "";
                break;
            default:
                cout << "Opcion invalida.\n";
                break;
        }

        cout << "\n";

    } while (op != 5);
}
//menu de supervisor (misma descripcion que el menu de empleado)
void menuSupervisor() {
    int op = 0;
    string input;

    do {
        cout << "=== Menu Supervisor ===\n\n";
        cout << "1. Ver timesheets de mi equipo\n";
        cout << "2. Aprobar timesheet\n";
        cout << "3. Rechazar timesheet\n";
        cout << "4. Cerrar sesion\n\n";
        cout << "Opcion: ";

        if (cin.peek() == '\n') {
            cin.get();
        }

        if (!getline(cin, input)) {
            return;
        }

        if (input.size() != 1 || input[0] < '1' || input[0] > '4') {
            cout << "\nDebes escribir un numero entre 1 y 4.\n\n";
            continue;
        }

        op = input[0] - '0';

        cout << "\n";

        switch (op) {
    case 1:
        verTimesheetsEquipo();
        break;
    case 2:
        aprobarTimesheet();
        break;
    case 3:
        rechazarTimesheet();
        break;
    case 4:
        cout << "Cerrando sesion de " << currentUsername << "...\n";
        isLoggedIn = false;
        currentUserId = -1;
        currentUsername = "";
        currentUserRole = "";
        break;
    default:
        cout << "Opcion invalida.\n";
        break;
}

        cout << "\n";

    } while (op != 4);
}

//para verifirca usuario y contrasena
void iniciarSesion(){
      string username;
    string password;

    cout << "=== Iniciar Sesion ===\n\n";
    cout << "Username: ";
    cin >> username;
    cout << "Password: ";
    cin >> password;

    ifstream inFile(USERS_FILE);
    if (!inFile.is_open()) {
        cout << "No se pudo abrir el archivo de usuarios.\n";
        return;
    }

    bool encontrado = false;

    int fileId;
    string fileUsername, filePasswordHash, fileRole;

    //todos los usuario en el archivo tiene id username password y role
    while (inFile >> fileId >> fileUsername >> filePasswordHash >> fileRole) {

    //calculamos el hash del password que el usuario acaba de escribir
    string inputPasswordHash = hashPassword(password);

    if (username == fileUsername && inputPasswordHash == filePasswordHash) {
            //si encontramos un match nos deja iniciar la sesion
            encontrado = true;

            isLoggedIn = true;
            currentUserId = fileId;
            currentUsername = fileUsername;
            currentUserRole = fileRole;

            cout << "Bienvenido " << currentUsername
                << " (" << currentUserRole << ")\n\n";

            //actualizamos el log 
            logAction(currentUserId,
                    currentUserRole,
                    "LOGIN",
                    "user",
                    currentUserId,
                    "success",
                    "login correcto");

            //accesamos por rol
            if (currentUserRole == "employee") {
                menuEmployee();
            } else if (currentUserRole == "supervisor") {
                menuSupervisor();
            } else {
                cout << "Rol desconocido.\n";
            }

            break;
        }
    }

    inFile.close();
//por si las credenciales no son match
    if (!encontrado) {
        cout << "\nCredenciales invalidas.\n";
        isLoggedIn = false;
        currentUserId = -1;
        currentUsername = "";
        currentUserRole = "";

        //actualiza el log como intento fallido con credenciales invalidas
    logAction(-1,
              "unknown",
              "LOGIN",
              "user",
              -1,
              "fail",
              "credenciales invalidas");
    }

}


int main() {
    int op = 0;
    string input;

    do{
        cout << "Menu Principal\n\n";
        cout << "1. Registrarse\n\n";
        cout << "2. Inciar Sesion\n\n";
        cout << "3. Salir\n\n";
        cout << "Elige una opcion: ";

        cin >> input;

        //valida que solamente sea un caracter entre el 1 2 y 3
        if (input.size() != 1 || (input[0] < '1' || input[0] > '3')) {
            cout << "\nDebes escribir un numero entre 1 y 3.\n\n";//mensaje de error y te devuelve al inicio
            continue;
        }

        //convertimos el string insertado en un int
        op = input[0] - '0';

        cout << "\n";

        switch(op){
            case 1:
                registrarUsuario();
                break;
            case 2:
                iniciarSesion();
                break;
            case 3:
                cout << "\nSaliendo...\n";
                break;
            default:
                cout << "Opcion invalida\n";
                break;
        }

        cout << "\n";

    } while(op != 3);

    return 0;
}