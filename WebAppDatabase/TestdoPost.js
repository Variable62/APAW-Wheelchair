function doGet() {
  return ContentService
    .createTextOutput("Backend Running");
}

function doPost(e) {

  var sheet = SpreadsheetApp
    .getActiveSpreadsheet()
    .getActiveSheet();

  var data = JSON.parse(e.postData.contents);

  var now = new Date();

  var dateString = Utilities.formatDate(
      now,
      Session.getScriptTimeZone(),
      "dd/MM/yyyy"
  );

  var timeString = Utilities.formatDate(
      now,
      Session.getScriptTimeZone(),
      "HH:mm:ss"
  );

  sheet.appendRow([
    dateString,
    timeString,

    data.state,
    data.pressure,

    data.fsr1,
    data.fsr2,
    data.fsr3,
    data.fsr4,
    data.fsr5,
    data.fsr6,

    data.pump,
    data.valve,

    data.gyx,
    data.gyy
  ]);

  return ContentService
      .createTextOutput("OK");
}

function testInsert() {

  var sheet = SpreadsheetApp
    .getActiveSpreadsheet()
    .getActiveSheet();

  var now = new Date();

  sheet.appendRow([
    Utilities.formatDate(now, "Asia/Bangkok", "dd/MM/yyyy"),
    Utilities.formatDate(now, "Asia/Bangkok", "HH:mm:ss"),

    "WARNING",
    1.25,

    250,
    260,
    270,
    280,
    290,
    300,

    "ON",
    "OFF",

    5.2,
    3.1
  ]);
}
