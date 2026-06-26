window.onload = () => {

    loadData();

    setInterval(loadData, REFRESH_INTERVAL);

};


async function loadData() {

    try {

        const response = await fetch(API_URL);

        const data = await response.json();

        if (!data || data.length === 0) {

            console.log("No data");

            return;

        }

        updateCurrentStatus(data[0]);

        updateHistoryTable(data);

    }

    catch (error) {

        console.error(error);

    }

}

function updateCurrentStatus(row) {

    document.getElementById("state").textContent = row[2];

    document.getElementById("pressure").textContent =
        row[3] + " PSI";

    document.getElementById("pump").textContent =
        row[10];

    document.getElementById("valve").textContent =
        row[11];

    document.getElementById("gyx").textContent =
        row[12] + "°";

    document.getElementById("gyy").textContent =
        row[13] + "°";

    document.getElementById("lastUpdate").textContent =
        row[0] + " " + row[1];

}

function updateHistoryTable(data) {

    const tbody = document.getElementById("tableBody");

    tbody.innerHTML = "";

    data.slice(0, MAX_ROWS).forEach(row => {

        const tr = document.createElement("tr");

        tr.className = row[2];

        tr.innerHTML = `

        <td>${row[0]}</td>
        <td>${row[1]}</td>
        <td>${row[2]}</td>
        <td>${row[3]}</td>

        <td>${row[4]}</td>
        <td>${row[5]}</td>
        <td>${row[6]}</td>
        <td>${row[7]}</td>
        <td>${row[8]}</td>
        <td>${row[9]}</td>

        <td>${row[10]}</td>
        <td>${row[11]}</td>

        <td>${row[12]}</td>
        <td>${row[13]}</td>

        `;

        tbody.appendChild(tr);

    });

}