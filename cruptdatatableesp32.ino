#include <FS.h>
#include <SD.h>
#include <ESPAsyncWebServer.h>

#define SD_CS_PIN 5

File dataFile;
String dataFileName = "/siswa.txt";  // Nama file untuk menyimpan data siswa

const char *ssid = "Pasti Dadi";
const char *password = "bejo0000";

struct Siswa {
  int id;
  String nama;
  String alamat;
  String nomor_tlp;
};

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Gagal menginisialisasi SD Card!");
    return;
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.5.0/css/bootstrap.min.css'>";
    html += "<link rel='stylesheet' type='text/css' href='https://cdn.datatables.net/1.10.21/css/jquery.dataTables.css'></head><body>";
    html += "<nav class='navbar navbar-expand-lg navbar-dark bg-dark'>";
    html += "<a class='navbar-brand' href='/'>IoT ESP32 Web Server Simpan Database SDCard</a>";
    html += "<ul class='navbar-nav'>";
    html += "</ul></div></nav>";

    html += "<div class='container'>";
    html += "<h1 class='mt-5'>Data Table Siswa</h1>";
    html += "<table id='siswaTable' class='table mt-3'>";
    html += "<thead><tr><th>ID</th><th>Nama</th><th>Alamat</th><th>Nomor Tlp</th><th>Action</th></tr></thead><tbody>";

    File file = SD.open(dataFileName);  // Menggunakan nama file yang sudah ditentukan
    if (file) {
      int id = 1;
      while (file.available()) {
        String line = file.readStringUntil('\n');
        Siswa siswa = parseSiswa(line);
        html += "<tr>";
        html += "<td>" + String(siswa.id) + "</td>";
        html += "<td>" + siswa.nama + "</td>";
        html += "<td>" + siswa.alamat + "</td>";
        html += "<td>" + siswa.nomor_tlp + "</td>";
        html += "<td><a class='btn btn-primary btn-sm' href='/edit?id=" + String(siswa.id) + "'>Edit</a> <a class='btn btn-danger btn-sm' href='/delete?id=" + String(siswa.id) + "'>Delete</a></td>";
        html += "</tr>";
      }
      file.close();
    }

    html += "</tbody></table>";
    html += "<a class='btn btn-success' href='/add'>Tambah Data</a>";
    html += "</div></body>";
    html += "<script src='https://code.jquery.com/jquery-3.5.1.min.js'></script>";
    html += "<script src='https://cdn.datatables.net/1.10.21/js/jquery.dataTables.js'></script>";
    html += "<script>";
    html += "$(document).ready( function () { $('#siswaTable').DataTable(); } );";
    html += "</script></html>";

    request->send(200, "text/html", html);
  });

  server.on("/add", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.5.0/css/bootstrap.min.css'></head><body>";
    html += "<div class='container'>";
    html += "<h1 class='mt-5'>Tambah Data</h1>";
    html += "<form action='/save' method='POST'>";
    html += "<div class='form-group'>";
    html += "<label for='nama'>Nama:</label>";
    html += "<input type='text' class='form-control' name='nama' required>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label for='alamat'>Alamat:</label>";
    html += "<input type='text' class='form-control' name='alamat' required>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label for='nomor_tlp'>Nomor Tlp:</label>";
    html += "<input type='text' class='form-control' name='nomor_tlp' required>";
    html += "</div>";
    html += "<button type='submit' class='btn btn-primary'>Simpan</button>";
    html += "</form>";
    html += "<br><a class='btn btn-secondary' href='/'>Kembali ke Home</a>";
    html += "</div></body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/edit", HTTP_GET, [](AsyncWebServerRequest *request) {
    String idParam = request->arg("id");
    int id = idParam.toInt();

    Siswa siswa = readSiswaById(id);

    String html = "<html><head><link rel='stylesheet' href='https://stackpath.bootstrapcdn.com/bootstrap/4.5.0/css/bootstrap.min.css'></head><body>";
    html += "<div class='container'>";
    html += "<h1 class='mt-5'>Edit Data</h1>";
    html += "<form action='/update' method='POST'>";
    html += "<input type='hidden' name='id' value='" + String(siswa.id) + "'>";
    html += "<div class='form-group'>";
    html += "<label for='nama'>Nama:</label>";
    html += "<input type='text' class='form-control' name='nama' value='" + siswa.nama + "'>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label for='alamat'>Alamat:</label>";
    html += "<input type='text' class='form-control' name='alamat' value='" + siswa.alamat + "'>";
    html += "</div>";
    html += "<div class='form-group'>";
    html += "<label for='nomor_tlp'>Nomor Tlp:</label>";
    html += "<input type='text' class='form-control' name='nomor_tlp' value='" + siswa.nomor_tlp + "'>";
    html += "</div>";
    html += "<button type='submit' class='btn btn-primary'>Update</button>";
    html += "</form>";
    html += "<br><a class='btn btn-secondary' href='/'>Kembali ke Home</a>";
    html += "</div></body></html>";
    request->send(200, "text/html", html);
  });

  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request) {
    String idParam = request->arg("id");
    int id = idParam.toInt();

    if (deleteSiswaById(id)) {
      Serial.println("Data siswa dengan ID " + String(id) + " berhasil dihapus");
    } else {
      Serial.println("Gagal menghapus data siswa dengan ID " + String(id));
    }

    // Redirect ke halaman home
    request->send(100, "text/plain", "See Other");
    request->redirect("/");
  });

  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    String nama = request->arg("nama");
    String alamat = request->arg("alamat");
    String nomor_tlp = request->arg("nomor_tlp");

    if (saveDataToSD(nama, alamat, nomor_tlp)) {
      Serial.println("Data berhasil disimpan ke SD Card");
    } else {
      Serial.println("Gagal menyimpan data ke SD Card");
    }

    // Redirect ke halaman home
    request->send(100, "text/plain", "See Other");
    request->redirect("/");
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    int id = request->arg("id").toInt();
    String nama = request->arg("nama");
    String alamat = request->arg("alamat");
    String nomor_tlp = request->arg("nomor_tlp");

    if (updateSiswa(id, nama, alamat, nomor_tlp)) {
      Serial.println("Data siswa dengan ID " + String(id) + " berhasil diupdate");
    } else {
      Serial.println("Gagal mengupdate data siswa dengan ID " + String(id));
    }

    // Redirect ke halaman home
    request->send(100, "text/plain", "See Other");
    request->redirect("/");
  });

  server.begin();
}

void loop() {
  // Loop program...

}

Siswa parseSiswa(String line) {
  Siswa siswa;
  int comma1 = line.indexOf(',');
  int comma2 = line.indexOf(',', comma1 + 1);
  int comma3 = line.indexOf(',', comma2 + 1);

  siswa.id = line.substring(0, comma1).toInt();
  siswa.nama = line.substring(comma1 + 1, comma2);
  siswa.alamat = line.substring(comma2 + 1, comma3);
  siswa.nomor_tlp = line.substring(comma3 + 1);

  return siswa;
}

bool saveDataToSD(String nama, String alamat, String nomor_tlp) {
  File file = SD.open(dataFileName, FILE_APPEND);
  if (file) {
    int id = countSiswa() + 1;
    file.print(id);
    file.print(",");
    file.print(nama);
    file.print(",");
    file.print(alamat);
    file.print(",");
    file.println(nomor_tlp);
    file.close();
    return true;
  }
  return false;
}

Siswa readSiswaById(int id) {
  File file = SD.open(dataFileName);
  Siswa siswa;

  if (file) {
    while (file.available()) {
      String line = file.readStringUntil('\n');
      Siswa currentSiswa = parseSiswa(line);
      if (currentSiswa.id == id) {
        siswa = currentSiswa;
        break;
      }
    }
    file.close();
  }

  return siswa;
}

bool updateSiswa(int id, String nama, String alamat, String nomor_tlp) {
  File file = SD.open(dataFileName);
  File tempFile = SD.open("/temp.txt", FILE_WRITE);

  bool updated = false;

  if (file && tempFile) {
    while (file.available()) {
      String line = file.readStringUntil('\n');
      Siswa currentSiswa = parseSiswa(line);
      if (currentSiswa.id == id) {
        tempFile.print(id);
        tempFile.print(",");
        tempFile.print(nama);
        tempFile.print(",");
        tempFile.print(alamat);
        tempFile.print(",");
        tempFile.println(nomor_tlp);
        updated = true;
      } else {
        tempFile.println(line);
      }
    }

    file.close();
    tempFile.close();

    if (updated) {
      SD.remove(dataFileName);
      SD.rename("/temp.txt", dataFileName);
    } else {
      SD.remove("/temp.txt");
    }
  }

  return updated;
}

bool deleteSiswaById(int id) {
  File file = SD.open(dataFileName);
  File tempFile = SD.open("/temp.txt", FILE_WRITE);

  bool deleted = false;

  if (file && tempFile) {
    while (file.available()) {
      String line = file.readStringUntil('\n');
      Siswa currentSiswa = parseSiswa(line);
      if (currentSiswa.id == id) {
        deleted = true;
      } else {
        tempFile.println(line);
      }
    }

    file.close();
    tempFile.close();

    if (deleted) {
      SD.remove(dataFileName);
      SD.rename("/temp.txt", dataFileName);
    } else {
      SD.remove("/temp.txt");
    }
  }

  return deleted;
}

int countSiswa() {
  int count = 0;
  File file = SD.open(dataFileName);
  if (file) {
    while (file.available()) {
      file.readStringUntil('\n');
      count++;
    }
    file.close();
  }
  return count;
}
