function doPost(e) {
  try {
    const jsonString = e.postData.contents;
    const data = JSON.parse(jsonString);
    
    const state        = data.state || "-";
    const fsr1         = data.fsr1 !== undefined ? data.fsr1 : 0;
    const fsr2         = data.fsr2 !== undefined ? data.fsr2 : 0;
    const fsr3         = data.fsr3 !== undefined ? data.fsr3 : 0;
    const fsr4         = data.fsr4 !== undefined ? data.fsr4 : 0;
    const fsr5         = data.fsr5 !== undefined ? data.fsr5 : 0;
    const fsr6         = data.fsr6 !== undefined ? data.fsr6 : 0;
    
    const status_pump  = data.pump_status || "-";
    const status_valve = data.valve_status || "-";
    const led_normal   = data.led_normal || "-";
    const led_warning  = data.led_warning || "-";
    const led_danger   = data.led_danger || "-";

    const now = new Date();
    const dateString = Utilities.formatDate(now, "Asia/Bangkok", "dd/MM/yyyy");
    const timeString = Utilities.formatDate(now, "Asia/Bangkok", "HH:mm:ss");
    
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
    
    sheet.appendRow([
      dateString,
      timeString,
      state,
      fsr1,
      fsr2,
      fsr3,
      fsr4,
      fsr5,
      fsr6,
      status_pump,
      status_valve,
      led_normal,
      led_warning,
      led_danger
    ]);
    
    return ContentService.createTextOutput(JSON.stringify({
      "result": "success",
      "message": "Data logged successfully"
    })).setMimeType(ContentService.MimeType.JSON);
      
  } catch(error) {
    return ContentService.createTextOutput(JSON.stringify({
      "result": "error", 
      "message": error.toString()
    })).setMimeType(ContentService.MimeType.JSON);
  }
}
