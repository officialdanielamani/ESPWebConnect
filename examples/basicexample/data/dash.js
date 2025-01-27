document.addEventListener("DOMContentLoaded", () => {
    // Get references to the dashboard and dropdown
    updateReadings();
    closeNotification();
    const dashboard = document.getElementById("dashboard");
    const widgetSelection = document.querySelector(".dropdown-content");

    if (!dashboard || !widgetSelection) {
        console.error("Dashboard or widget selection not found.");
        return;
    }

    // Track active widgets
    const activeWidgets = new Set();

    // Add a new widget
    function addWidget(id, name, type, icon = "fa fa-question", color = "#d4aa4f", desc, unit) {
        if (!dashboard) return;

        // Prevent duplicates
        if (document.querySelector(`#${id}`)) {
            alert(`${name} [${id}] is already on the dashboard.`);
            return;
        }

        const widget = document.createElement("div");
        widget.className = "widget";
        widget.id = id;
        widget.dataset.type = type;
        widget.desc = desc;
        widget.unit = unit;

        widget.style.gridColumnStart = 1;
        widget.style.gridRowStart = 1;
        widget.style.gridColumnEnd = "span 4";
        widget.style.gridRowEnd = "span 4";
        const gID = id.toLowerCase();

        // Content generation
        if (type === "switch") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-name">${name}</div>
                <div class="widget-desc">${desc}</div>
            </div>
            <label class="switch">
                <input type="checkbox" id="${gID}-val" onclick="toggleSwitch('${gID}-val','${id}')">
                <span class="switch_sw"></span>
            </label>
        `;
        } else if (type === "sensor") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-name">${name}</div>
                <div class="widget-desc">${desc}</div>
            </div>
            <div class="sensor-value">
                <span class="widget-value" id="${gID}-val">0.00</span>
                <div class="widget-unit">${unit}</div>
            </div>
        `;
        }
        else if (type === "button") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-desc">${desc}</div>
            </div>
            <button id="${id}" type="submit" onclick="pressButton('${id}')">${name}</button>
        `;
        } else if (type === "inputnumber") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-name">${name}</div>
                <div class="widget-desc">${desc}</div>
            </div>
            <div class="widget-form">
            <form onsubmit="return submitInput('${id}')">
                <input type="number" id="${id}" name="value">
                <button type="submit">OK</button>
            </form>
            </div>
        `;
        } else if (type === "inputtext") {
            widget.innerHTML = `
            <div class="card-info">
                <div class="widget-icon"><i class="${icon}" style="color: ${color};"></i></div>
                <div class="widget-name">${name}</div>
                <div class="widget-desc">${desc}</div>
            </div>
            <div class="widget-form">
            <form onsubmit="return submitInput('${id}')">
                <input type="text" id="${id}" name="value">
                <button type="submit">OK</button>
            </form>
            </div>
        `;
        }

        // Remove button
        const removeBtn = document.createElement("button");
        removeBtn.className = "remove-btn";
        removeBtn.textContent = "X";
        removeBtn.onclick = () => removeWidget(id);
        widget.appendChild(removeBtn);

        // Add resizer
        const resizer = document.createElement("div");
        resizer.className = "resizer";
        widget.appendChild(resizer);

        // Add drag-and-resize functionality
        enableDragAndResize(widget);

        // Add to the dashboard
        dashboard.appendChild(widget);
        activeWidgets.add(id);
    }

    // Remove a widget
    function removeWidget(id) {
        const widget = document.getElementById(id);
        if (widget) {
            widget.remove();
            activeWidgets.delete(id);
        }
    }

    // Drag and drop
    function enableDragAndResize(widget) {
        let isDragging = false;
        let isResizing = false;
        let startX, startY, initialWidth, initialHeight;

        const maxColumns = 32; // Maximum grid columns
        const maxRows = 32; // Maximum grid rows
        const gridWidth = 50; // Width of each grid cell
        const gridHeight = 50; // Height of each grid cell

        // Dragging logic
        const startDrag = (e) => {
            if (isResizing) return;
            isDragging = true;
            widget.classList.add("dragging");
            const touch = e.touches ? e.touches[0] : e;
            startX = touch.clientX;
            startY = touch.clientY;

            const moveDrag = (e) => {
                if (!isDragging) return;
                const touch = e.touches ? e.touches[0] : e;
                const rect = dashboard.getBoundingClientRect();

                // Calculate new grid positions
                let column = Math.max(1, Math.ceil((touch.clientX - rect.left) / gridWidth));
                let row = Math.max(1, Math.ceil((touch.clientY - rect.top) / gridHeight));

                // Constrain to grid boundaries
                column = Math.min(column, maxColumns - parseInt(widget.style.gridColumnEnd.split("span")[1]));
                row = Math.min(row, maxRows - parseInt(widget.style.gridRowEnd.split("span")[1]));

                widget.style.gridColumnStart = column;
                widget.style.gridRowStart = row;
            };

            const stopDrag = () => {
                isDragging = false;
                widget.classList.remove("dragging");
                document.removeEventListener("mousemove", moveDrag);
                document.removeEventListener("mouseup", stopDrag);
                document.removeEventListener("touchmove", moveDrag);
                document.removeEventListener("touchend", stopDrag);
            };

            document.addEventListener("mousemove", moveDrag);
            document.addEventListener("mouseup", stopDrag);
            document.addEventListener("touchmove", moveDrag);
            document.addEventListener("touchend", stopDrag);
        };

        widget.addEventListener("mousedown", startDrag);
        widget.addEventListener("touchstart", startDrag);

        // Resizing logic
        const startResize = (e) => {
            e.preventDefault();
            isResizing = true;
            const touch = e.touches ? e.touches[0] : e;
            startX = touch.clientX;
            startY = touch.clientY;

            const rect = widget.getBoundingClientRect();
            initialWidth = rect.width;
            initialHeight = rect.height;

            const moveResize = (e) => {
                if (!isResizing) return;
                const touch = e.touches ? e.touches[0] : e;
                const dx = touch.clientX - startX;
                const dy = touch.clientY - startY;

                // Calculate new spans
                let newWidth = Math.max(1, Math.ceil((initialWidth + dx) / gridWidth));
                let newHeight = Math.max(1, Math.ceil((initialHeight + dy) / gridHeight));

                // Constrain spans to grid boundaries
                newWidth = Math.min(newWidth, maxColumns - parseInt(widget.style.gridColumnStart));
                newHeight = Math.min(newHeight, maxRows - parseInt(widget.style.gridRowStart));

                widget.style.gridColumnEnd = `span ${newWidth}`;
                widget.style.gridRowEnd = `span ${newHeight}`;
            };

            const stopResize = () => {
                isResizing = false;
                document.removeEventListener("mousemove", moveResize);
                document.removeEventListener("mouseup", stopResize);
                document.removeEventListener("touchmove", moveResize);
                document.removeEventListener("touchend", stopResize);
            };

            document.addEventListener("mousemove", moveResize);
            document.addEventListener("mouseup", stopResize);
            document.addEventListener("touchmove", moveResize);
            document.addEventListener("touchend", stopResize);
        };

        const resizer = widget.querySelector(".resizer");
        resizer.addEventListener("mousedown", startResize);
        resizer.addEventListener("touchstart", startResize);
    }

    // Event delegation for dropdown
    widgetSelection.addEventListener("click", (e) => {
        const button = e.target.closest("button");
        if (!button || !button.dataset.widget) return;

        const id = button.dataset.widget;
        const name = button.dataset.name || "Unknown Widget";
        const type = button.dataset.type || "sensor";
        const icon = button.dataset.icon || "fa fa-question";
        const color = button.dataset.color || "#d4aa4f";
        const desc = button.dataset.desc || "";
        const unit = button.dataset.unit || "";

        addWidget(id, name, type, icon, color, desc, unit);
    });

    function loadDashboard() {
        const savedState = JSON.parse(localStorage.getItem("dashboardSettings"));
        if (!savedState) return;
    
        const validWidgets = Array.from(widgetSelection.querySelectorAll("button")).map(
            (btn) => btn.dataset.widget
        );
    
        savedState.widgets.forEach((widgetData) => {
            if (!validWidgets.includes(widgetData.id)) return;
    
            // Add the widget with saved properties
            addWidget(
                widgetData.id,
                widgetData.name || widgetData.id, // Use saved name or fallback to ID
                widgetData.type,
                widgetData.icon, // Pass saved icon
                widgetData.color, // Pass saved color
                widgetData.desc, // Pass saved description
                widgetData.unit // Pass saved unit
            );
    
            // Restore widget size and position
            const widget = document.getElementById(widgetData.id);
            if (widget) {
                widget.style.gridColumnStart = widgetData.position.columnStart;
                widget.style.gridColumnEnd = widgetData.position.columnEnd;
                widget.style.gridRowStart = widgetData.position.rowStart;
                widget.style.gridRowEnd = widgetData.position.rowEnd;
            }
        });
    
        if (savedState.theme) {
            document.body.dataset.theme = savedState.theme;
        }
    }
    
    
    document.querySelector('button[onclick="saveDashboard()"]').addEventListener("click", saveDashboard);
    document.querySelector('button[onclick="clearDashboard()"]').addEventListener("click", clearDashboard);

    loadDashboard();
});

function clearDashboard() {
    localStorage.removeItem("dashboardSettings");
    dashboard.innerHTML = "";
    alert("Dashboard cleared!");
}

function saveDashboard() {
    const widgets = Array.from(dashboard.querySelectorAll(".widget"));
    const savedWidgets = widgets.map((widget) => {
        let name = widget.querySelector('.btn-widget')?.innerText || // Check for button widgets
                   widget.querySelector('.widget-name')?.innerText || // Check for name in a dedicated div
                   widget.querySelector('button[type="submit"]')?.innerText ||
                   widget.id; // Fallback to widget ID if no name found

        return {
            id: widget.id,
            name: name, // Save the determined name
            type: widget.dataset.type || "undefined",
            icon: widget.querySelector('.widget-icon i')?.className || "fa fa-question", // Extract icon class
            color: widget.querySelector('.widget-icon i')?.style.color || "#d4aa4f", // Extract color
            desc: widget.querySelector('.widget-desc')?.innerText || "", // Extract description
            unit: widget.querySelector('.widget-unit')?.innerText || "", // Extract unit
            position: {
                columnStart: widget.style.gridColumnStart,
                columnEnd: widget.style.gridColumnEnd,
                rowStart: widget.style.gridRowStart,
                rowEnd: widget.style.gridRowEnd,
            },
        };
    });

    const theme = document.body.dataset.theme || "default";
    const dashboardState = { widgets: savedWidgets, theme };

    localStorage.setItem("dashboardSettings", JSON.stringify(dashboardState));
    alert("Dashboard saved!");
}

// ###Core widget functions###

function toggleSwitch(gID, id) {
    // Get the checkbox element by its ID
    var checkbox = document.querySelector(`input#${gID}`);
    if (!checkbox) {
        console.error(`Checkbox with ID '${gID}' not found`);
        return;
    }

    // Send the state of the checkbox
    var xhr = new XMLHttpRequest();
    var state = checkbox.checked ? 'true' : 'false'; // Use 'on' and 'off' for clarity
    xhr.open('GET', `/toggleSwitch?id=${id}&state=${state}`, true);
    xhr.send();
}

// Send button
function pressButton(id) {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', '/pressButton?id=' + id, true);
    xhr.send();
}

// Add JavaScript for handling form submissions via AJAX (simplified version)
function submitInput(id) {
    // Get the input element within the form
    const inputElement = document.querySelector(`form[onsubmit="return submitInput('${id}')"] input`);
    
    if (!inputElement) {
        console.error(`Input field for id '${id}' not found.`);
        return false;
    }

    const value = inputElement.value;

    fetch('/' + id, {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: 'value=' + encodeURIComponent(value) // Send the input value
    })
        .then(response => response.text())
        .then(text => {
            console.log(text);
            alert('Value updated successfully');
        })
        .catch(error => {
            console.error('Error:', error);
            alert('Failed to update value');
        });

    return false; // Prevent form submission
}


// Establish WebSocket connection
const webSocket = new WebSocket(`ws://${window.location.hostname}/ws`);

// Handle incoming WebSocket messages
webSocket.onmessage = (event) => {
    const { id, message, messageColor, icon, iconColor, timeout } = JSON.parse(event.data);
    showNotification(message, messageColor, icon, iconColor, timeout);
};

// Function to display notification
function showNotification(message, messageColor = '', icon = '', iconColor = '', timeout = 0) {
    const banner = document.getElementById('notificationBanner');
    const messageElement = document.getElementById('notificationMessage');
    const iconElement = document.getElementById('notificationIcon');

    // Set message and icon text content and color
    messageElement.textContent = message;
    if (messageColor) {
        messageElement.style.color = messageColor; // Set message text color
        iconElement.style.color = messageColor;    // Set icon color
    }

    // Set icon class
    if (icon) {
        iconElement.className = `notification-icon ${icon}`;
    }

    // Set banner background color
    if (iconColor) {
        banner.style.backgroundColor = iconColor; // Set banner background color
    }

    // Show the banner
    banner.style.display = 'flex';

    // Auto-hide after timeout (if specified)
    if (timeout > 0) {
        setTimeout(() => {
            banner.style.display = 'none';
        }, timeout * 1000);
    }
}


function closeNotification() {
    const banner = document.getElementById('notificationBanner');
    banner.style.removeProperty('display'); // Removes inline display
    banner.style.display = 'none'; // Sets to none
}



// ## Theme Selection ##

function changeTheme(event) {
    const selectedTheme = event.target.value; // Get the value of the clicked button

    // Update the data-theme attribute on the body to change the theme
    document.body.setAttribute('data-theme', selectedTheme);
}
