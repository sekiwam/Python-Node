
class A {
    constructor() {
        this.a = 5;
    }
}

const AB = new Proxy(A, {
    construct: function (target, args) {
        let a = new A();
        a.a = 30;
        return a;
    }
});

console.info(new AB());









const target = {
    message1: "hello",
    message2: "everyone"
};

const handler2 = {
    get: function (target, prop, receiver) {
        console.info(`prop ${prop}`)
        return "world";
    }
};

const proxy2 = new Proxy(target, handler2);

console.info(proxy2.wow)
console.info(proxy2[0])


function extend(sup, base) {
    var descriptor = Object.getOwnPropertyDescriptor(
        base.prototype, 'constructor'
    );
    base.prototype = Object.create(sup.prototype);
    var handler = {
        construct: function (target, args) {
            var obj = Object.create(base.prototype);
            this.apply(target, obj, args);
            return obj;
        },
        apply: function (target, that, args) {
            sup.apply(that, args);
            base.apply(that, args);
        }
    };
    var proxy = new Proxy(base, handler);
    descriptor.value = proxy;
    Object.defineProperty(base.prototype, 'constructor', descriptor);
    return proxy;
}

var Person = function (name) {
    this.name = name;
};

var Boy = extend(Person, function (name, age) {
    this.age = age;
});

Boy.prototype.gender = 'M';

var Peter = new Boy('Peter', 13);

console.log(Peter.gender);  // "M"
console.log(Peter.name);    // "Peter"
console.log(Peter.age);     // 13