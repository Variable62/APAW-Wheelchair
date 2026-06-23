function doPost(e) {
  try {
    const jsonString = e.postData.contents;
    const data = JSON.parse(jsonString);
    
    const state       = data.values.state || "-";
    const cloudTime   = data.values.Time || 0;
    const fsr1        = data.values.fsr1 !== undefined ? data.values.fsr1 : 0;
    const fsr2        = data.values.fsr2 !== undefined ? data.values.fsr2 : 0;
    const fsr3        = data.values.fsr3 !== undefined ? data.values.fsr3 : 0;
    const fsr4        = data.values.fsr4 !== undefined ? data.values.fsr4 : 0;
    const fsr5        = data.values.fsr5 !== undefined ? data.values.fsr5 : 0;
    const fsr6        = data.values.fsr6 !== undefined ? data.values.fsr6 : 0;
    
    const status_pump  = data.values.pump_status ? "ON" : "OFF";
    const status_valve = data.values.valve_status ? "ON" : "OFF";
    const led_normal   = data.values.led_normal ? "ON" : "OFF";
    const led_warning  = data.values.led_warning ? "ON" : "OFF";
    const led_danger   = data.values.led_danger ? "ON" : "OFF";

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
