function doPost(e) {
  try {
    const jsonString = e.postData.contents;
    const data = JSON.parse(jsonString);
    const rawLog = data.values.sitting_log; 
    
    if (!rawLog) {
      return ContentService.createTextOutput("No valid log data found.").setMimeType(ContentService.MimeType.TEXT);
    }
    
    const fields = rawLog.split(",");
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
    
    sheet.appendRow([
      new Date(), 
      fields[0],  
      fields[1],  
      fields[2],  
      fields[3],  
      fields[4]   
    ]);
    
    const lastRow = sheet.getLastRow();
    const startRow = Math.max(2, lastRow - 3); 
    
    let summaryText = "⏱️ ประวัติประเมินล่าสุด:\n-------------------------\n";
    
    for (let i = lastRow; i >= startRow; i--) {
      let timeStart = sheet.getRange(i, 2).getValue().toString(); 
      let duration  = sheet.getRange(i, 3).getValue().toString(); 
      let statusStr = sheet.getRange(i, 6).getValue().toString(); 
      
      summaryText += `📌 ${timeStart} | นั่ง ${duration} นาที | [${statusStr}]\n`;
    }
    
    return ContentService.createTextOutput(JSON.stringify({
      "result": "success",
      "history_table": summaryText  
    })).setMimeType(ContentService.MimeType.JSON);
      
  } catch(error) {
    return ContentService.createTextOutput(JSON.stringify({
      "result": "error", 
      "message": error.toString()
    })).setMimeType(ContentService.MimeType.JSON);
  }
}
